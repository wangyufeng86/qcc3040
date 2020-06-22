# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
#   %%version
import string
import re

fh = open("host_msg.txt", 'r')
for line in fh:
    print "UPGRADE_HOST_" + line.rstrip() + ","
fh.close()

print

fh = open("host_err.txt", 'r')
for line in fh:
    print "UPGRADE_HOST_" + line.rstrip() + ","
fh.close()

print

fh = open("host_msg.txt", 'r')
for line in fh:
    print "typedef UPGRADE_HOST_" + line.rstrip() + " " + "UPGRADE_HOST_" + line.rstrip() + "_T;"
fh.close()

print

class StateName:
    upper = ""
    cap = ""

state_list = list()

fh = open("states.txt", 'r')
for line in fh:
    state_name = StateName()
    state_name.upper = line.rstrip()
    state_name.cap = re.sub(r"_", r"", string.capwords(line.rstrip(), '_'))
    state_list.append(state_name)
fh.close()

for state in state_list:
    print "static void Handle" + state.cap + "(MessageId id);"
    
for state in state_list:
    print "case UPGRADE_STATE_" + state.upper + ":"
    print "    Handle" + state.cap + "(id);"
    print "    break;"
    print
    
for state in state_list:
    print "void Handle" + state.cap + "(MessageId id)"
    print "{"
    print
    print "}"
    print
    
    