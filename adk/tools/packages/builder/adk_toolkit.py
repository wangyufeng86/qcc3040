import os

try:
    from scandir import walk
except ImportError:
    from os import walk


class AdkToolkit(object):
    def __init__(self, adk_toolkit_path=None):
        if not adk_toolkit_path:
            try:
                adk_toolkit_path = os.environ['ADK_ROOT']
            except KeyError:
                raise EnvironmentError("ERROR: ADK Toolkit path not in environment")

        if not os.path.isdir(adk_toolkit_path):
            raise FileNotFoundError("ADK Toolkit path doesn't exist: {path}".format(path=adk_toolkit_path))

        self.root = os.path.normpath(adk_toolkit_path)
        self.cache = {}

    def __getitem__(self, name):
        try:
            return self.cache[name]
        except KeyError:
            for root, folders, files in walk(self.root):
                for each in files + folders:
                    if name == each:
                        self.cache[name] = os.path.normpath(os.path.abspath(os.path.join(root, each)))
                        return self.cache[name]
            else:
                raise
