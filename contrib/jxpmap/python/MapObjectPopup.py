import gtk
from EditorDialog import EditorDialog

class MapObjectPopup:
    def __init__(self, object, event, canvas):
        items = (
            ("/Bring to front", None, self.toFront, 0, None),
            ("/Send to back", None, self.toBack, 0, None),
            ("/Remove", None, self.remove, 0, None),
            ("/Properties", None, self.properties, 0, None)
            )

        # Would be nicer to create the menu only once and then show/hide the
        # same one for each object
        self.object = object
        self.canvas = canvas
        accel_group = gtk.GtkAccelGroup()
        item_factory = gtk.GtkItemFactory(gtk.GtkMenu.get_type(), "<main>",
                                          accel_group)
        item_factory.create_items(items)
        menu = item_factory.get_widget("<main>")
        menu.popup(None, None, None, event.button, event.time)

    def toFront(self, data, menuitem):
        self.canvas.getModel().removeObject(self.object)
        self.canvas.getModel().addToFront(self.object)
        self.canvas.repaint()
        self.canvas.saveUndo()

    def toBack(self, data, menuitem):
        self.canvas.getModel().removeObject(self.object)
        self.canvas.getModel().addToBack(self.object)
        self.canvas.repaint()
        self.canvas.saveUndo()

    def remove(self, data, menuitem):
        self.canvas.getModel().removeObject(self.object)
        self.canvas.repaint()
        self.canvas.saveUndo()

    def properties(self, data, menuitem):
        EditorDialog(self.canvas, self.object)
