#!python

""" Get an xpath from an xml input file """

import xml.dom.minidom
import xml.etree.ElementTree as ET
try:
    from itertools import ifilter as filter  # pylint: disable=redefined-builtin
except ImportError:
    pass
from commander import runner, Commander
from utils import fprint, open_for_writing, suffix


class XPathCommand(Commander):
    """ xpath commands """
    # Deriving from Commander means any method who's name starts with do_ is a command
    # line option and it's docstring is the help and the number of arguments is
    # dynamically looked up with the inspect module

    def __init__(self, prog_name):
        """ ctor """
        super(XPathCommand, self).__init__(self, prog_name)
        self.infile = None
        self.tree = None
        self.root = None
        self.filters = []
        self.xml_pi = {}  # pylint: disable=invalid-name

    def _find_one(self, search, tree=None):
        """ find all and assert one hit """
        ret = (tree or self.tree).findall(search)
        assert ret and len(ret) == 1
        return ret[0]

    def _named_items(self, xpath, select=None):
        """ items matching an xpath, that have an attribute called "name" """
        if select and not isinstance(select, (list, tuple)):
            select = [select]
        for item in self.tree.findall(xpath):
            try:
                name = item.attrib["name"]
                if select:
                    if name in select:
                        yield name, item
                else:
                    yield name, item
            except KeyError:
                pass

    def _items(self, xpath, select=None):
        """ items matching an xpath """
        if select and not isinstance(select, (list, tuple)):
            select = [select]
        for item in self.tree.findall(xpath):
            if select:
                if item.tag in select:
                    yield item
            else:
                yield item

    def _folder(self, select):
        """ get the files in a folder """
        for _name, item in self._named_items(".//folder", select):
            for child in item:
                if child.tag == "file" and "path" in child.attrib:
                    path = child.attrib.get("path")
                    if path:
                        yield path

    @staticmethod
    def _processing_instruction(infile):
        """ Get the processing instruction from the file """
        with open(infile) as fob:
            for line in fob:
                line = line.strip()
                if line.startswith("<?") and line.endswith("?>"):
                    return line[2:-2]
        return None

    @staticmethod
    def _xml_pi(infile):
        """ Get the xml processing instruction from the file """
        ret = {}
        pi_line = XPathCommand._processing_instruction(infile)
        if pi_line and pi_line.startswith("xml "):
            for part in pi_line.split():
                if "=" in part:
                    key, val = part.split("=")
                    if val and len(val) > 2 and val[0] == "\"" and val[-1] == "\"":
                        val = val[1:-1]
                    ret[key] = val
            return ret
        return ret

    def do_input(self, infile):
        """ supply the input file used in other commands """
        self.infile = infile
        self.xml_pi = self._xml_pi(self.infile)
        self.tree = ET.parse(self.infile)
        assert self.tree
        self.root = self.tree.getroot()

    def do_folder(self, name):
        """ get the named folder paths """
        for path in self._folder(name):
            fprint(path)

    def do_devkitgroup(self):
        """ get devkitgroup value from a project """
        item = next(self._items(".//configuration/", select="devkitGroup"))
        if item is not None:
            fprint(item.text)

    def do_configs(self, name):
        """ transform the named configuration property into gcc-like -Dkey=val list """
        for _name, item in self._named_items(".//configuration/property", name):
            if item.text:
                defs = ["-D%s" % define.strip() for define in item.text.split(" ") if define.strip()]
                fprint(" ".join(defs))

    @staticmethod
    def _props(tree):
        """ get property elements that have a name attribute """
        for elem in tree.findall(".//property[@name]"):
            yield elem

    @staticmethod
    def _properties(tree):
        """ Get the name and text of the properties """
        for elem in XPathCommand._props(tree):
            yield elem.attrib["name"], (elem.text or "")

    @staticmethod
    def _files(tree, filters=None):
        """ get all file elements that have a path attribute (apply filters) """
        elems = [item for item in tree.findall(".//file[@path]")]
        if filters:
            for filt in filters:
                elems = filter(filt, elems)
        for elem in elems:
            yield elem

    @staticmethod
    def _file_paths(tree, filters=None):
        """ get path attributes of file elements (can be filtered further """
        for elem in XPathCommand._files(tree, filters=filters):
            yield elem.attrib["path"]

    def do_file_paths(self):
        """ get path attributes of file elements (can be filtered further """
        fprint("\n".join(XPathCommand._file_paths(self.tree, self.filters)))

    def _attr_present(self, attrib):
        """ restrict file_paths() to just elements with the attribute """
        self.filters.append(lambda elem: attrib in elem.attrib)

    def _attr_missing(self, attrib):
        """ restrict file_paths() to just elements without the attribute """
        self.filters.append(lambda elem: attrib not in elem.attrib)

    def _attr_eq(self, name, value):
        """ restrict file_paths() to the elements where the attrib is present and equal to value """
        self._attr_present(name)
        self.filters.append(lambda elem: elem.attrib[name] == value)

    def _attr_ne(self, name, value):
        """ restrict file_paths() to the elements where the attrib is present and not equal to value """
        self._attr_present(name)
        self.filters.append(lambda elem: elem.attrib[name] != value)

    @staticmethod
    def _merge_text(elem, text, sep=" "):
        """ merge text in elem with supplied text """
        new_text = text.strip()
        if not new_text:
            # incoming (stripped) text it blank, nothing to do
            return
        elem_text = (elem.text or "").strip()
        if elem_text:
            # existing text exists (and stripped, is not blank), merge new text, preserving order
            elem_words = elem_text.split(sep)
            new_words = new_text.strip().split(sep)
            to_add = [val for val in new_words if val not in elem_words]
            newtext = sep.join(elem_words + to_add)
        else:
            # existing text is blank, just use incoming text
            newtext = new_text
        # no matter what, the element's new text is stripped
        elem.text = newtext.strip()

    def merge_x2p(self, to_merge):
        """ Read an x2p XML file, merge it with the root node from --input """
        merge_tree = ET.parse(to_merge)
        # input_root = merge_tree.getroot()
        paths = set(self._file_paths(self.tree))
        for elem in XPathCommand._files(merge_tree):
            path = elem.attrib["path"]
            if path not in paths:
                self.root.append(elem)
        input_props = dict(XPathCommand._properties(merge_tree))
        for prop in self._props(self.tree):
            name = prop.attrib["name"]
            if name in input_props:
                XPathCommand._merge_text(prop, input_props[name])

    @staticmethod
    def _elem_equal(lhs, rhs):
        """ Are the elements equal? """
        if lhs.tag != rhs.tag:
            return False
        if lhs.text != rhs.text:
            return False
        if lhs.attrib != rhs.attrib:
            return False
        if len(lhs) != len(rhs):
            return False
        return True

    @staticmethod
    def _node_has_child(parent, elem):
        """ Does the parent already have a child element? """
        for child in parent:
            if XPathCommand._elem_equal(elem, child):
                return True
        return False

    def merge_x2w(self, to_merge):
        """ Read an x2w XML file, merge it with the root node from --input """
        merge_tree = ET.parse(to_merge)
        workspace = self._find_one(".")
        assert workspace.tag == "workspace"
        dep_node = self._find_one(".//project[@default='yes']/dependencies")
        for proj in merge_tree.findall(".//project/dependencies/project"):
            if not XPathCommand._node_has_child(dep_node, proj):
                dep_node.append(proj)
        for proj in merge_tree.findall(".//project[@default='no']"):
            if not XPathCommand._node_has_child(workspace, proj):
                workspace.append(proj)

    def do_no_export(self):
        """ restrict file_paths() to just paths without an export attribute """
        self._attr_missing("export")

    def do_export(self, value):
        """ restrict file_paths() to just paths with an export attribute equal to "value" """
        self._attr_eq("export", value)

    def do_suffix(self, suff):
        """ restrict file_paths() to just paths with a suffix of suff """
        if not suff.startswith("."):
            suff = "." + suff
        self.filters.append(lambda elem: elem.attrib["path"].endswith(suff))

    def do_headers(self):
        """ restrict file_paths() to headers """
        self.do_suffix("h")

    def do_sources(self):
        """ restrict file_paths() to sources """
        self.do_suffix("c")

    def do_merge(self, fname):
        """ Read another XML file, merge it with the root node from --input (input suffix dictates the logic used) """
        if "." in self.infile:
            suff = suffix(self.infile)
            if suff in XPathCommand.MERGERS:
                XPathCommand.MERGERS[suff](self, fname)
            else:
                raise RuntimeWarning("Cannot merge to input file with suffix '%s'" % suff)
        else:
            raise RuntimeWarning("Cannot merge to input file with no suffix")

    def do_pretty(self, fname):
        """ pretty print the in-memry XML tree to output fname ("-" means stdout) """
        lines = ET.tostringlist(self.root)
        dom = xml.dom.minidom.parseString("".join(l for l in lines if l and l.strip()))
        pretty_xml = dom.toprettyxml(indent="    ", encoding=self.xml_pi.get("encoding", None))
        if fname == "-":
            fprint(pretty_xml, end="")
        else:
            with open_for_writing(fname, "b") as fob:
                fprint(pretty_xml, end="", file=fob)

    def do_output(self, fname):
        """ an alias for --pretty """
        self.do_pretty(fname)

    MERGERS = {"x2p": merge_x2p, "x2w": merge_x2w}


if __name__ == "__main__":
    runner(XPathCommand)
