from MapModel import MapModel
from MapCanvas import MapCanvas
from MapFuel import MapFuel
from MapPolygon import MapPolygon
from MapBase import MapBase
from MapBall import MapBall
from MapCheckPoint import MapCheckPoint
import gtk
import GDK

class MainFrame:
    def __init__(self):
        self.canvas = None
        self.window = gtk.GtkWindow(gtk.WINDOW_TOPLEVEL)
        self.window.connect("delete_event", self.delete_event)
        self.window.set_title("jXPMap Editor")
        self.box0 = gtk.GtkVBox(gtk.FALSE, 0)
        menubar = self.buildMenuBar()
        self.box0.pack_start(menubar, gtk.FALSE, gtk.FALSE, 0)
        self.box1 = gtk.GtkHBox(gtk.FALSE, 0)
        self.box0.pack_start(self.box1, gtk.TRUE, gtk.TRUE, 0)
        self.window.add(self.box0)
        self.buildToolBar()
        #self.ActionMap()
        #self.buildInputMap()

        # The visual calls are needed to make sure drawing bitmaps
        # (draw_*_image) works
        # ... at least according to some documentation that doesn't look
        # entirely reliable

        gtk.push_rgb_visual()
        area = gtk.GtkDrawingArea()
        gtk.pop_visual()
        area.size(500, 500)
        self.box1.pack_start(area, gtk.TRUE, gtk.TRUE, 0)
        area.connect("expose-event", self.area_expose_cb)

        # Change the default background to black so that the window
        # doesn't flash badly when resizing

        style = area.get_style().copy()
        style.bg[gtk.STATE_NORMAL] = area.get_colormap().alloc(0, 0, 0)
        area.set_style(style)
        area.connect("button_press_event", self.mouse_event)
        area.connect("button_release_event", self.mouse_event)
        area.connect("motion_notify_event", self.mouse_event)
        area.connect("configure_event", self.configure_event)
        area.set_events(GDK.EXPOSURE_MASK | GDK.BUTTON_PRESS_MASK |
                        GDK.BUTTON_RELEASE_MASK | GDK.POINTER_MOTION_MASK |
                        GDK.POINTER_MOTION_HINT_MASK)

        # The HINT_MASK means there won't be a callback for every mouse
        # motion event, only after certain other events or after an explicit
        # reading of the location with x, y = window.pointer.
        # This helps to avoid getting a queue of events if they're not handled
        # quickly enough.

        area.show()
        self.zoom = 0
        self.box1.show()
        self.box0.show()
        self.window.show()
        self.canvas = MapCanvas(area)

    def mouse_event(self, area, event):
        if event.type == GDK.BUTTON_PRESS and event.button == 5:
            self.canvas.setCenter(event.x, event.y)
            self.canvas.paint(None)
        else:
            self.canvas.handle(area, event)
        return gtk.TRUE

    def area_expose_cb(self, area, event):
        self.canvas.paint(event.area)
        return gtk.TRUE

    def delete_event(self, widget, event):
        gtk.mainquit()
        return gtk.FALSE

    def configure_event(self, widget, event):
        if self.canvas is not None:
            self.canvas.resize()

    def buildMenuBar(self):
        menu_items = (
            ("/File", None, None, 0, "<Branch>"),
            ("/File/New", None, self.newMap, 0, None),
            ("/File/Open", None, self.openMap, 0, None),
            ("/File/Save", None, self.saveMap, 0, None),
            )
        accel_group = gtk.GtkAccelGroup()
        item_factory = gtk.GtkItemFactory(gtk.GtkMenuBar.get_type(), "<main>",
                                          accel_group)
        item_factory.create_items(menu_items)
        menubar = item_factory.get_widget("<main>")
        menubar.show()
        return menubar

    def buildToolBar(self):
        box = gtk.GtkVBox(gtk.FALSE, 0)
        l = [["New polygon", MapPolygon], ["New fuel", MapFuel],
             ["New checkpoint", MapCheckPoint], ["New ball", MapBall],
             ["New base", MapBase]]
        l = [[a, self.newObject, b] for a, b in l]
        l.extend([["Erase", self.toggleEraseMode, None],
                  ["Toggle points", self.canvasToggle, 'showPoints'],
                  ["Toggle filled", self.canvasToggle, 'filled'],
                  ["Toggle textured", self.canvasToggle, 'textured'],
                  ["Undo", self.undo, -1], ["Redo", self.undo, 1],
                  ["Zoom in", self.zoom, 1], ["Zoom out", self.zoom, -1]
                  ])

        for name, callback, data in l:
            button = gtk.GtkButton(name)
            button.connect("clicked", callback, data)
            box.pack_start(button, gtk.FALSE, gtk.FALSE, 0)
            button.show()
        self.zoomlabel = gtk.GtkLabel("1.00")
        self.zoomlabel.show()
        box.pack_start(self.zoomlabel, gtk.FALSE, gtk.FALSE, 0)
        self.box1.pack_start(box, gtk.FALSE, gtk.FALSE, 0)
        box.show()

    def undo(self, widget, value):
        self.canvas.undo(value)

    def newObject(self, widget, Type):
        self.canvas.setErase(0)
        self.canvas.setCanvasEventHandler(Type(bmstore = self.canvas.bmstore).
                                          getCreateHandler())

    def zoom(self, widget, value):
        self.zoom += value
        if self.zoom > 20:
            self.zoom = 20
            return
        elif self.zoom < -15:
            self.zoom = -15
            return
        self.updateScale()

    def canvasToggle(self, widget, data):
        setattr(self.canvas, data, not getattr(self.canvas, data))
        self.canvas.repaint()

    def toggleEraseMode(self, widget, data):
        self.canvas.setCanvasEventHandler(None)
        self.canvas.setErase(not self.canvas.isErase())

    def updateScale(self):
        z = 1.3**self.zoom
        self.zoomlabel.set_text("%.2f" % z)
        z *= self.canvas.getModel().getDefaultScale()
        self.canvas.setScale(z)
        self.canvas.repaint()

    def setModel(self, model):
        self.canvas.setModel(model)

    def newMap(self, menuitem = None, data = None):
        self.mapFile = None
        self.setModel(MapModel())
        #showOptions()

    def openMap(self, menuitem, data):
        print 'Not opening map'

    def saveMap(self, menuitem, data):
        file = open('tempfile', 'w')
        self.canvas.getModel().save(file)

    def main(self):
        gtk.mainloop()

if __name__ == '__main__':
    mf = MainFrame()
    import sys
    if len(sys.argv) <= 1:
        mf.newMap()
    else:
        model = MapModel()
        file = open(sys.argv[1])
        model.load(file, mf.canvas.bmstore)
        mf.setModel(model)
    mf.main()
