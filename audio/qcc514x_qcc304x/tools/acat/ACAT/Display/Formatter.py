############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2012 - 2018 Qualcomm Technologies, Inc. and/or its
# subsidiaries. All rights reserved.
#
############################################################################
"""
Module for defining the Formatter interface.
"""
try:
    from future_builtins import hex
except ImportError:
    pass


class Formatter(object):
    """defines an interface for outputting analysis data.

    A typical analysis might do something like::

      self.formatter.section_start('Widget Frobnication status')
      self.formatter.output('Widget is:')
      self.formatter.output(self.get_widget())
      if self.widget_is_clearly_corrupt:
          self.formatter.alert('Corrupt widget!')
      self.formatter.section_end()

    """

    def __init__(self):
        # Sets processor number.
        self.proc = "P0"

    def section_start(self, header_str):
        """Starts a new section. Sections can be nested.

        Args:
            header_str
        """
        raise NotImplementedError()

    def section_end(self):
        """End a section."""
        raise NotImplementedError()

    def section_reset(self):
        """Reset the section formatting, ending all open sections.

        This is provided so that in case of an error we can continue safely.
        """
        raise NotImplementedError()

    def set_proc(self, proc):
        """Sets the processor.

        Args:
            proc: Current processor being analysed
        """
        self.proc = proc

    def output(self, string_to_output):
        """Normal body text. Lists/dictionaries will be compacted.

        Args:
            string_to_output
        """
        raise NotImplementedError()

    def output_svg(self, string_to_output):
        """The svg body text. Lists/dictionaries will be compacted.

        Args:
            string_to_output (str)
        """
        raise NotImplementedError()

    def output_raw(self, string_to_output):
        """Unformatted text output. Useful when displaying tables.

        Args:
            string_to_output
        """
        raise NotImplementedError()

    def output_list(self, string_to_output):
        """Normal body text. Lists/dictionaries will be printed in long-form.

        Args:
            string_to_output (str)
        """
        if isinstance(string_to_output, (list, tuple)):
            # Less-easy
            # Printing to the screen is slow, so do it as one big string to go
            # faster
            list_string = []
            for t in string_to_output:
                list_string.append(self._prepare_output(t))

            # Generate a multi line plain-text
            self.output('\n'.join(list_string))

        else:
            # Easy
            self.output(string_to_output)

    def alert(self, alert_str):
        """Raise an alert.

        Important information that we want to be highlighted. For example,
        ``pmalloc pools exhausted`` or ``chip has panicked``.

        Args:
            alert_str (str)
        """
        raise NotImplementedError()

    def error(self, error_str):
        """Raise an error.

        This signifies some problem with the analysis tool itself, e.g. an
        analysis can't complete for some reason.

        Args:
            error_str
        """
        raise NotImplementedError()

    def flush(self):
        """Output all logged events.

        Events such as body text, alerts, errors etc. will be saved into the
        given file.
        """
        raise NotImplementedError()

    def _prepare_output(self, output):
        """Recursive method to prepare the given output.

        Args:
            output: This can be any type but ultimately it should be
                possible to cast it to string.

        Returns:
            String
        """
        if isinstance(output, (tuple, list)):
            prepared_output = []
            for item in output:
                prepared_output.append(self._prepare_output(item))

            return str(prepared_output)
        else:

            try:
                return hex(output)
            except TypeError:
                return str(output)
