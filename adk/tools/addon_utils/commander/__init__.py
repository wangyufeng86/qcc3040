#!python

""" Base class for writing self-documenting command-line tools """

# All function in the supplied class (a derived class I assume) that start with the string "do_"
# will be discovered automatically and their docstrings are their help docs
# The signatures are inspected and the nubmer of arguments to pop off argv is discovered with "inspect"

from __future__ import print_function
import sys
import os
import inspect
from collections import OrderedDict
from utils import fprint

__all__ = ["runner", "Commander"]


def runner(klass):
    """ make an instance and execute it """
    run = klass(sys.argv[0])
    run.execute(sys.argv[1:])
    sys.exit(run.status)


# pylint: disable not equal useless-object-inheritance,too-few-public-methods
class Commander(object):  # pylint: too-few-public-methods
    """ Run commands """
    def __init__(self, child, prog_name):
        members = list(inspect.getmembers(child, inspect.ismethod))
        members = sorted(members, key=lambda t: t[1].__code__.co_firstlineno)
        self._cmds = [m[0] for m in members if m[0].startswith("do_")]
        self._funcs = OrderedDict((cmd, getattr(child, cmd)) for cmd in self._cmds)
        self.prog_name = os.path.abspath(prog_name)
        self.status = 0

    def execute(self, argv):
        """ interpret the args """
        args = argv or ["help"]
        while args:
            cmd = args.pop(0)
            if cmd.startswith("--"):
                cmd = cmd[2:]
            fname = "do_%s" % cmd
            if fname in self._cmds:
                func, arg_names = self._func_args(fname)
                if len(arg_names) <= len(args):
                    cmd_args = [args.pop(0) for _ in arg_names]
                    self.status = func(*cmd_args) or 0
                else:
                    fprint("Not enough arguments given for command %s (got %s, need %s)" % (cmd, len(args), len(arg_names)), file=sys.stderr)
                    self.status = -1
                    return
            else:
                fprint("Command %s is not a known command!" % cmd, file=sys.stderr)
                self.do_help()
                self.status = -1
                return

    def _func_args(self, fname):
        """ Get the argument names that a do_* function takes (elides self) """
        func = self._funcs[fname]
        try:
            arg_names = inspect.getfullargspec(func).args[1:]
        except AttributeError:
            # pylint: disable=deprecated-method
            arg_names = inspect.getargspec(func).args[1:]
        return func, arg_names

    def __nonzero__(self):  # Python 2.7
        """ return a boolean """
        return self.status == 0

    def __bool__(self):  # Python 3
        """ return a boolean """
        return self.status == 0

    def do_help(self):
        """ Show this help """
        keywords = [cmd[3:] for cmd in self._cmds if cmd != "do_help"]
        fprint("%s: %s" % (os.path.basename(self.prog_name), ", ".join("--%s" % kw for kw in keywords)))

        def help_cmd(cmd):
            """ helper """
            keyword = cmd[3:]
            func, arg_names = self._func_args(cmd)
            argc = len(arg_names)
            doc = func.__doc__.strip()
            if argc == 1:
                fprint("--%s: %s (takes 1 argument: %s)" % (keyword, doc, arg_names[0]))
            elif argc > 1:
                fprint("--%s: %s (takes %s arguments: %s)" % (keyword, doc, argc, ", ".join(arg_names)))
            else:
                fprint("--%s: %s" % (keyword, doc))
        for cmd in self._cmds:
            if cmd != "do_help":
                help_cmd(cmd)
        fprint("")
        help_cmd("do_help")
