#!python

""" General utilities """

from __future__ import print_function
import sys
import os
import stat
import errno
from contextlib import contextmanager

__all__ = ["fprint", "open_for_writing", "suffix"]


try:
    PermissionError
except NameError:
    class PermissionError(Exception):  # pylint: disable=redefined-builtin
        """ dummy """


def suffix(fname):
    """ cleaner wrapper to splitexit() """
    ret = None
    if "." in fname:
        ret = os.path.splitext(fname)[1][1:]
    return ret


def fprint(*args, **kwargs):
    """ flushing print """
    print(*args, **kwargs)
    kwargs.get("file", sys.stdout).flush()


def ferr(*args, **kwargs):
    """ flush print to stderr """
    fprint(*args, file=sys.stderr, **kwargs)


@contextmanager
def open_for_writing(fname, othermode=None):
    """ make sure the file is writable """
    wmode = "w%s" % (othermode or "")
    write = stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH | stat.S_IWRITE
    try:
        try:
            with open(fname, wmode) as fob:
                yield fob
        except IOError as err:
            if err.errno in [errno.EACCES, errno.EPERM]:
                raise PermissionError()
            raise
    except PermissionError:
        old_mode = os.stat(fname).st_mode
        os.chmod(fname, write)
        with open(fname, wmode) as fob:
            yield fob
        os.chmod(fname, old_mode)
