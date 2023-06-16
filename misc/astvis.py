import gdb
import graphviz

class ASTVis(gdb.Command):
    def __init__(self):
        super(ASTVis, self).__init__("astvis", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, tty):
        val = gdb.parse_and_eval(arg)
        if val.type.name != "Term" and val.type.name != "Term*":
            print("astvis requires a Term or Term*")
        print(val['next'])
ASTVis()