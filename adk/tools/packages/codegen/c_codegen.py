'''
 Copyright (c) 2017-2019 Qualcomm Technologies International, Ltd.
'''

from __future__ import print_function

class Indented(object):
    ''' Maintains the current indent level. 
        Use indented_print() to print text at the correct current indent level. '''
    def __init__(self, outfile=None):
        self.outfile = outfile

    level = 0
    spaces_per_indent = 4
    def __enter__(self):
        Indented.level += 1
        return self
    def __exit__(self, type, value, traceback):
        assert Indented.level >= 0
        Indented.level -= 1
    @classmethod
    def indent(cls):
        return ' ' * cls.spaces_per_indent * cls.level

    def indented_print(self, string):
        print(self.indent() + string, file=self.outfile)

    def ignore_indent_print(self, string):
        print(string, file=self.outfile)


class CDefinition(Indented):
    ''' Base class for creating enums, structs, unions '''
    def __init__(self, type, name, array=False, typedef=False, eol=",", doc=""):
        self.type = type
        self.name = name
        self.array = array
        self.typedef = typedef
        self.doc = doc
        self.eol = eol
        self.list = []
        self.docs = []
        super(CDefinition, self).__init__()

    def __enter__(self):
        ''' On entry, print the start of the CDefinition '''
        CommentDoxy().add(self.doc)
        self.indented_print('{}{} {}{}\n{{'.format('typedef ' if self.typedef else '',
                                                   self.type,
                                                   '' if self.typedef else self.name,
                                                   '[] =' if self.array else ''))
        return super(CDefinition, self).__enter__()

    def __exit__(self, type, value, traceback):
        ''' On exit, print the items and complete '''
        items = len(self.list)
        for i in range(items):
            if len(self.docs):
                CommentDoxy().add(self.docs.pop(0))
            print(self.indent() + self.list.pop(0) + self.eol)
        super(CDefinition, self).__exit__(type, value, traceback)
        self.indented_print('}} {};\n'.format(self.name if self.typedef else ''))

    def extend(self, items):
        self.list.extend(items)

    def extend_with_doc(self, items, docs):
        assert(len(items) == len(docs))
        assert(len(self.list) == len(self.docs))
        self.list.extend(items)
        self.docs.extend(docs)


class Enumeration(CDefinition):
    ''' Generate a c enumeration '''
    def __init__(self, name, doc="", eol=","):
        CDefinition.__init__(self, 'enum', name, doc=doc, eol=eol)

class EnumerationTypedef(CDefinition):
    ''' Generate a c enumeration '''
    def __init__(self, name, doc=""):
        CDefinition.__init__(self, 'enum', name, typedef=True, doc=doc)

class Array(CDefinition):
    ''' Generate a c array '''
    def __init__(self, array_type, name, doc="", eol=","):
        CDefinition.__init__(self, array_type, name, array=True, doc=doc, eol=eol)

class StructTypedef(CDefinition):
    ''' Generate a c struct typedef '''
    def __init__(self, name, doc=""):
        CDefinition.__init__(self, 'struct', name, typedef=True, eol=";", doc=doc)

class UnionTypedef(CDefinition):
    ''' Generate a c union typedef '''
    def __init__(self, name, doc=""):
        CDefinition.__init__(self, 'union', name, typedef=True, eol=";", doc=doc)

class CommentDoxy(Indented):
    ''' Generate a c comment '''
    def add(self, comment):
        if comment:
            self.indented_print("/*! {} */".format(comment))

class CommentBlock(Indented):
    ''' Generate a c comment '''
    def __enter__(self):
        self.indented_print('/*')
        return super(CommentBlock, self).__enter__()
    def __exit__(self, type, value, traceback):
        super(CommentBlock, self).__exit__(type, value, traceback)
        self.indented_print('*/\n')

class CommentBlockDoxygen(Indented):
    ''' Use this class to generate doxygen c comments '''
    def __enter__(self):
        self.indented_print('/*!')
        return super(CommentBlockDoxygen, self).__enter__()
    def __exit__(self, type, value, traceback):
        super(CommentBlockDoxygen, self).__exit__(type, value, traceback)
        self.indented_print('*/\n')
    def doxy_copyright(self):
        from datetime import datetime
        year = datetime.now().year
        self.indented_print("\\copyright Copyright (c) {} Qualcomm Technologies International, Ltd.".format(year))
        with Indented():
            self.indented_print("All Rights Reserved.")
            self.indented_print("Qualcomm Technologies International, Ltd. Confidential and Proprietary.")
    def doxy_version(self):
        self.indented_print("\\version %%version")
    def doxy_filename(self, filename):
        self.indented_print("\\file {}".format(filename))
    def doxy_brief(self, brief):
        self.indented_print("\\brief {}".format(brief))
    def doxy_page(self, page_title):
        self.ignore_indent_print("\\page {}".format(page_title))

class HeaderGuards(object):
    ''' Use this class to print header file guards '''
    def __init__(self, filename_base, outfile=None):
        self.outfile = outfile
        self.guard_name = '_{}_H__'.format(filename_base.upper())
    def __enter__(self):
        print('#ifndef {}'.format(self.guard_name), file=self.outfile)
        print('#define {}\n'.format(self.guard_name), file=self.outfile)
    def __exit__(self, type, value, traceback):
        print('#endif /* {} */\n'.format(self.guard_name), file=self.outfile)

class Include(object):
    ''' Use this class to #include a list of files '''
    def __init__(self, include_file_list, outfile=None):
        for f in include_file_list:
                print("#include <{}>".format(f), file=outfile)
        print('')
