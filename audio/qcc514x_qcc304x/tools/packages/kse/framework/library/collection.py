#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Collections library utilities

All function in this module handle json objects
A json object is the representation of valid json in python.
The object supports dict, OrderedDict and list containers and any values.
Subelements could be containers as well.
'''

import copy
import os
from string import Template

from six import string_types

from .config import get_config_param

OUT_MODE_INPLACE = 'inplace'
OUT_MODE_SHALLOW_COPY = 'shallow_copy'
OUT_MODE_DEEP_COPY = 'deep_copy'
OUT_MODES = [OUT_MODE_INPLACE, OUT_MODE_SHALLOW_COPY, OUT_MODE_DEEP_COPY]

OUT_MODE_FUNC = {
    OUT_MODE_INPLACE: lambda x: x,
    OUT_MODE_SHALLOW_COPY: copy.copy,
    OUT_MODE_DEEP_COPY: copy.deepcopy

}

DICT_MODE_OVERWRITE = 'overwrite'
DICT_MODE_APPEND = 'append'
DICT_MODES = [DICT_MODE_OVERWRITE, DICT_MODE_APPEND]

LIST_MODE_OVERWRITE = 'overwrite'
LIST_MODE_APPEND = 'append'
LIST_MODE_OVERWRITE_APPEND = 'overwrite_append'
LIST_MODE_APPEND_ALWAYS = 'append_always'
LIST_MODES = [
    LIST_MODE_OVERWRITE,
    LIST_MODE_APPEND,
    LIST_MODE_OVERWRITE_APPEND,
    LIST_MODE_APPEND_ALWAYS]

SEPARATOR_DEFAULT = '.'


def json_merge(data1, data2, **kwargs):
    '''
    Merge two json objects.

    This function will return data1 data (base) with whatever modifications included in data2.
    The decision to add more data in data1 that was not originally present but present in data2 is
    defined by dict_mode and list_mode

    Args:
        data1 (dict, OrderedDict or list): Base input data
        data2 (dict, OrderedDict or list): New data
        out_mode (str). How to return data, set it to
            - OUT_MODE_INPLACE all operation will be done in data1
            - OUT_MODE_SHALLOW_COPY all operations will be done in a shallow copy of data1
            - OUT_MODE_DEEP_COPY all operations will be done in a deep copy of data1
        dict_mode (str). How to handle dict and OrderedDict
            - DICT_MODE_OVERWRITE keys that are already present in data1 will be overwritten with
              data2 if they exist, but new keys in data2 will be omitted.
            - DICT_MODE_APPEND keys only in data1 will be left intact, both in data1 and data2 will
              return data2 values and only in data2 will return data2 values
        list_mode (str). How to handle list
            - LIST_MODE_OVERWRITE only indexes in data1 will be overwritten with data2 if they exist
              in both data1 and data2
            - LIST_MODE_APPEND indexes only in data1 will be left intact, both in data1 and data2
              will return data2 values and only in data2 will return data2 values
            - LIST_MODE_OVERWRITE_APPEND values in data2 will overwrite existing indices in data1
              and new indices in data2 will append
            - LIST_MODE_APPEND_ALWAYS values in data2 will be appended to values in data1

    Returns:
        dict OrderedDict or list: Output data
    '''
    out_mode = kwargs.pop('out_mode', OUT_MODE_INPLACE)
    dict_mode = kwargs.pop('dict_mode', DICT_MODE_APPEND)
    list_mode = kwargs.pop('list_mode', LIST_MODE_OVERWRITE_APPEND)
    rec_kwargs = {
        'out_mode': out_mode,
        'dict_mode': dict_mode,
        'list_mode': list_mode,
    }

    if out_mode not in OUT_MODES:
        raise RuntimeError('out_mode:%s invalid')
    if dict_mode not in DICT_MODES:
        raise RuntimeError('dict_mode:%s invalid')
    if list_mode not in LIST_MODES:
        raise RuntimeError('list_mode:%s invalid')

    data = OUT_MODE_FUNC.get(out_mode)(data1)

    if isinstance(data2, dict):
        if isinstance(data, dict):
            for key in data2:
                if key in data:
                    data[key] = json_merge(data[key], data2[key], **rec_kwargs)
                elif dict_mode == DICT_MODE_APPEND:
                    data[key] = data2[key]
        else:
            return OUT_MODE_FUNC.get(out_mode)(data2)
    elif isinstance(data2, list):
        if isinstance(data, list):
            for index, value in enumerate(data2):
                if list_mode == LIST_MODE_APPEND_ALWAYS:
                    data.append(value)
                elif len(data) > index:
                    if list_mode in [LIST_MODE_OVERWRITE, LIST_MODE_OVERWRITE_APPEND]:
                        data[index] = json_merge(data[index], value, **rec_kwargs)
                elif list_mode in [LIST_MODE_OVERWRITE_APPEND, LIST_MODE_APPEND]:
                    data.append(value)
        else:
            return OUT_MODE_FUNC.get(out_mode)(data2)
    else:
        return OUT_MODE_FUNC.get(out_mode)(data2)
    return data


def json_substitute_env(data):
    '''
    Substitute string values that contain $ENV or $(ENV) with the value of the environmental
    variable

    .. code-block:: python

        json_substitute_env({'a': '$TMP/trash'})
        {'a': '/tmp/trash' }

    Args:
        data (dict, OrderedDict or list): Input data

    Returns:
        dict OrderedDict or list: Output data
    '''
    if isinstance(data, dict):
        for key in data:
            data[key] = json_substitute_env(data[key])
    elif isinstance(data, list):
        for index, value in enumerate(data):
            data[index] = json_substitute_env(value)
    elif isinstance(data, string_types):
        str_template = Template(data)
        env_data = copy.deepcopy(dict(os.environ))
        env_data = json_merge(env_data, get_config_param())
        return str_template.safe_substitute(**env_data)
    return data


def json_flatten(val, separator=SEPARATOR_DEFAULT, object_pairs_hook=None):
    '''
    Flatten a dictionary or a list

    All elements will be handled as scalars but subclasses of dicts and/or lists

    For the following input

        {
            "a": 1,
            "b": {
                "c": 1,
                "d": [0, 1, 2]
            }
        }

    Get the following flattened output

        {
            "a": 1,
            "b.c": 1,
            "b.d.0": 0,
            "b.d.1": 1,
            "b.d.2": 2
        }

    Args:
        data (list/dict): Data to flatten
        separator (str): Separator for returned flattened data dict
        object_pairs_hook (class): Class to use for output, by default dict()

    Returns:
        dict Flattened data
    '''
    object_pairs_hook = dict if not object_pairs_hook else object_pairs_hook
    if isinstance(val, dict):
        tmp = object_pairs_hook()
        for key in val:
            ret = json_flatten(val[key], separator=separator, object_pairs_hook=object_pairs_hook)
            if isinstance(ret, dict):
                for entry in ret:
                    tmp[key + separator + entry] = ret[entry]
            else:
                tmp[key] = ret
    elif isinstance(val, list):
        tmp = object_pairs_hook()
        for index, value in enumerate(val):
            ret = json_flatten(value, separator=separator, object_pairs_hook=object_pairs_hook)
            if isinstance(ret, dict):
                for entry in ret:
                    tmp[str(index) + separator + entry] = ret[entry]
            else:
                tmp[str(index)] = ret
    else:
        tmp = val
    return tmp


def json_unflatten(val, separator=SEPARATOR_DEFAULT, object_pairs_hook=None):
    '''
    Flatten a dictionary or a list.

    For the following input

        {
            "a": 1,
            "b.c": 1,
            "b.d.0": 0,
            "b.d.1": 1,
            "b.d.2": 2
        }

    Get the following unflattened output

        {
            "a": 1,
            "b": {
                "c": 1,
                "d": [0, 1, 2]
            }
        }

    Args:
        data (dict): Flatenned data
        separator (str): Separator for input flattened data dict
        object_pairs_hook (class): Class to use for dicts, by default dict()

    Returns:
        dict/list: Unflattened data
    '''

    def _check_list_length(root, data, separator=SEPARATOR_DEFAULT):
        comp = root + separator + str(0) if root else str(0)
        for entry in data:
            if entry == comp or entry.startswith(comp + separator):
                break
        else:
            raise ValueError('list %s does not have zero index' % (root))

        base = root.split(separator)
        length = len(base) if root else 0
        last = 0
        for entry in data:
            if entry.startswith(root):
                try:
                    index = int(entry.split(separator)[length])
                    last = last if index < last else index
                except Exception:  # pylint:disable=broad-except
                    pass
        return last + 1

    def _get_list_index(entry):
        pos = None
        try:
            pos = int(entry)
        except Exception:  # pylint:disable=broad-except
            pass
        return pos

    object_pairs_hook = dict if not object_pairs_hook else object_pairs_hook
    data = None
    for key in val:
        data2 = data  # we create a copy of the reference in order not to lose track of it
        entries = key.split(separator)
        for ind, ent in enumerate(entries[:-1]):
            pos = _get_list_index(ent)
            current_dict = pos is None
            pos_next = _get_list_index(entries[ind + 1])
            next_dict = pos_next is None
            next_ent = entries[ind + 1]

            if current_dict and next_dict:  # current dict next dict
                if data2 is None:
                    data2 = object_pairs_hook()
                    data = data2
                if not ent in data2 or data2[ent] is None:
                    data2[ent] = object_pairs_hook()
                if not next_ent in data2[ent]:
                    data2[ent][next_ent] = None
                data2 = data2[ent]

            elif current_dict and not next_dict:  # current dict next list
                if data2 is None:
                    data2 = object_pairs_hook()
                    data = data2
                if not ent in data2 or data2[ent] is None:
                    data2[ent] = [None] * _check_list_length(
                        separator.join(entries[:ind + 1]), val, separator=separator)
                data2 = data2[ent]

            elif not current_dict and not next_dict:  # current list next list
                if data2 is None:
                    data2 = [None] * _check_list_length(
                        separator.join(entries[:ind]), val, separator=separator)
                    data = data2
                if data2[pos] is None:
                    data2[pos] = [None] * _check_list_length(
                        separator.join(entries[:ind + 1]), val, separator=separator)
                data2 = data2[pos]

            elif not current_dict and next_dict:  # current list next dict
                if data2 is None:
                    data2 = [None] * _check_list_length(
                        separator.join(entries[:ind]), val, separator=separator)
                    data = data2
                if data2[pos] is None:
                    data2[pos] = object_pairs_hook()
                if not next_ent in data2[pos]:
                    data2[pos][next_ent] = None
                data2 = data2[pos]

        ent = entries[-1]
        pos = _get_list_index(ent)
        current_list = pos is not None and pos >= 0
        if not current_list:  # dictionary
            if data2 is None:
                data2 = object_pairs_hook()
                data = data2
            data2[ent] = val[key]
        else:  # list
            if data2 is None:
                data2 = [None] * _check_list_length(
                    separator.join(entries[:-1]), val, separator=separator)
                data = data2
            data2[pos] = val[key]

    return data
