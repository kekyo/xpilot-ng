from __future__ import nested_scopes # required in Python 2.1
import gtk

OK_CANCEL = 0
CLOSE = 1

class EditorDialog(gtk.GtkDialog):
    def __init__(self, canvas, object):
        gtk.GtkDialog.__init__(self)
        editor = object.getPropertyEditor(canvas)
        self.set_title("Property editor")
        self.vbox.pack_start(editor)
        def addbutton(text, function):
            b = gtk.GtkButton(text)
            b.connect("clicked", function)
            self.action_area.pack_start(b)
            b.show()
        def ok(button):
            if not editor.apply():
                return
            self.hide()
            self.destroy()
        def cancel(button):
            if not editor.cancel():
                return
            self.hide()
            self.destroy()
        if editor.buttons == OK_CANCEL:
            addbutton("OK", ok)
            addbutton("Cancel", cancel)
        else:
            addbutton("Close", ok)
        # gtk.grab_add(self) # modal
        self.show()
