#
# Copyright (c) 2019 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Template engine support
'''


def substitute(tmpl, test_dict):
    """
    This helper substitutes the templated values of `template` from
    matching keys in `test_dict`

    Args:
        tmpl (dict): The templated json config file to be used as the
                  source. The placeholders should start with
                  `TEMPLATE`.
        test_dict (dict): Dict with keys matching the placeholders in the
                   template as keys

    Returns:
        dict: A dict with all the placeholders in the template json filled
              with the values in test_dict
    """

    def is_string(tmpl):
        """
        An internal helper for `isinstance` to work on both python 2
        and 3 environments
        """
        try:
            return isinstance(tmpl, basestring)
        except NameError:
            return isinstance(tmpl, str)

    if is_string(tmpl) and 'TEMPLATE' in tmpl:
        return test_dict[tmpl]

    if isinstance(tmpl, dict):
        for key, _ in tmpl.items():
            tmpl[key] = substitute(tmpl[key], test_dict)
    elif isinstance(tmpl, list):
        for index, value in enumerate(tmpl):
            tmpl[index] = substitute(value, test_dict)

    return tmpl
