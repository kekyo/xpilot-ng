package org.xpilot.jxpmap;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.Shape;
import java.awt.RenderingHints;
import java.awt.event.MouseEvent;
import java.awt.event.InputEvent;
import java.awt.geom.AffineTransform;
import java.math.BigDecimal;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map;

import javax.swing.JComponent;
import javax.swing.undo.AbstractUndoableEdit;
import javax.swing.undo.CompoundEdit;
import javax.swing.undo.UndoManager;
import javax.swing.undo.UndoableEdit;

public class MapCanvas extends JComponent {
    
    public static final int MODE_SELECT = 0;
    public static final int MODE_EDIT = 1;
    public static final int MODE_ERASE = 2;
    public static final int MODE_COPY = 3;

    private MapModel model;
    private float scale;
    private AffineTransform at, it;
    private CanvasEventHandler eventHandler;
    private CanvasEventHandler dragHandler, selectHandler;
    private int mode;
    private Point offset;
    private UndoManager undoManager;
    private MapEdit currentEdit;
    private boolean fastRendering;
    private List selected;
    private int grid;

    public MapCanvas() {

        setOpaque(true);
        offset = new Point(0, 0);
        AwtEventHandler handler = new AwtEventHandler();
        addMouseListener(handler);
        addMouseMotionListener(handler);
        undoManager = new UndoManager();
        dragHandler = new DragHandler();
        selectHandler = new SelectHandler();
        selected = new ArrayList();
        grid = -1;
        mode = MODE_SELECT;
    }

    public void setCanvasEventHandler(CanvasEventHandler h) {
        eventHandler = h;
    }

    public CanvasEventHandler getCanvasEventHandler() {
        return eventHandler;
    }

    public UndoManager getUndoManager() {
        return undoManager;
    }

    public MapModel getModel() {
        return model;
    }
    
    public void setFastRendering(boolean value) {
        this.fastRendering = value;
    }
        
    public boolean isFastRendering() {
        return this.fastRendering;
    }
    
    public List getSelectedObjects() {
        return selected;
    }
    
    public void setSelectedObject(MapObject object) {
        if (!selected.isEmpty()) {
            for (Iterator i = selected.iterator(); i.hasNext();)
                ((MapObject)i.next()).setSelected(false);
        }
        selected.clear();
        if (object != null) {
            object.setSelected(true);
            selected.add(object);
        }
    }
    
    public void addSelectedObject(MapObject object) {
        object.setSelected(true);
        selected.add(object);
    }

    public void setModel(MapModel m) {
        this.model = m;
        this.scale = m.getDefaultScale();
        this.at = null;
        this.it = null;
        eventHandler = null;
        repaint();
    }

    public float getScale() {
        return scale;
    }

    public void setScale(float s) {
        this.offset.x = (int)(this.offset.x * s / this.scale);
        this.offset.y = (int)(this.offset.y * s / this.scale);
        this.scale = s;
        this.at = null;
        this.it = null;
        eventHandler = null;
        revalidate();
    }
    
    public void setMode(int mode) {
        this.mode = mode;
    }
    
    public int getMode() {
        return mode;
    }

    public void setGrid(int grid) {
        this.grid = grid;
    }
    
    public int getGrid() {
        return grid;
    }
            
    public void paint(Graphics _g) {

        if (model == null)
            return;

        Graphics2D g = (Graphics2D) _g;
        if (fastRendering)
            g.setRenderingHint(RenderingHints.KEY_RENDERING,
                RenderingHints.VALUE_RENDER_SPEED);
        Dimension mapSize = model.options.getSize();

        g.setColor(Color.black);
        g.fill(g.getClipBounds());

        AffineTransform rootTx = new AffineTransform(g.getTransform());
        rootTx.concatenate(getTransform());
        g.setTransform(rootTx);

        g.setColor(Color.red);
        Rectangle world = new Rectangle(0, 0, mapSize.width, mapSize.height);
        g.draw(world);
        //drawShapeNoTx(g, world);

        Rectangle view = g.getClipBounds();
        Point min = new Point();
        Point max = new Point();

        for (int i = model.objects.size() - 1; i >= 0; i--) {
            MapObject o = (MapObject) model.objects.get(i);
            computeBounds(min, max, view, o.getBounds(), mapSize);

            for (int xoff = min.x; xoff <= max.x; xoff++) {
                for (int yoff = min.y; yoff <= max.y; yoff++) {

                    AffineTransform tx = new AffineTransform(rootTx);
                    tx.translate(xoff * mapSize.width, yoff * mapSize.height);
                    g.setTransform(tx);
                    o.paint(g, scale);
                }
            }
        }
        g.setTransform(rootTx);
    }

    public void drawShape(Graphics2D g, Shape s) {
        if (g.getClipBounds() == null)
            g.setClip(getBounds());
        AffineTransform origTx = g.getTransform();
        AffineTransform rootTx = new AffineTransform(origTx);
        rootTx.concatenate(getTransform());
        g.setTransform(rootTx);
        drawShapeNoTx(g, s);
        g.setTransform(origTx);
    }

    private void drawShapeNoTx(Graphics2D g, Shape s) {

        Rectangle view = g.getClipBounds();
        AffineTransform origTx = g.getTransform();
        Point min = new Point();
        Point max = new Point();
        Dimension mapSize = model.options.getSize();

        computeBounds(min, max, view, s.getBounds(), mapSize);

        AffineTransform tx = new AffineTransform();
        for (int xoff = min.x; xoff <= max.x; xoff++) {
            for (int yoff = min.y; yoff <= max.y; yoff++) {
                tx.setTransform(origTx);
                tx.translate(xoff * mapSize.width, yoff * mapSize.height);
                g.setTransform(tx);
                g.draw(s);
            }
        }

        g.setTransform(origTx);
    }

    private void computeBounds(
        Point min,
        Point max,
        Rectangle view,
        Rectangle b,
        Dimension map) {
        if (model.options.isEdgeWrap()) {            
            min.x = (view.x - (b.x + b.width)) / map.width;
            if (view.x > b.x + b.width)
                min.x++;
            max.x = (view.x + view.width - b.x) / map.width;
            if (view.x + view.width < b.x)
                max.x--;
            min.y = (view.y - (b.y + b.height)) / map.height;
            if (view.y > b.y + b.height)
                min.y++;
            max.y = (view.y + view.height - b.y) / map.height;
            if (view.y + view.height < b.y)
                max.y--;
        } else {
            min.x = min.y = max.x = min.y = 0;
        }
    }

    public Point[] computeWraps(Rectangle r, Point p) {
        Dimension mapSize = model.options.getSize();
        Point min = new Point();
        Point max = new Point();
        Rectangle b = new Rectangle(p.x, p.y, 0, 0);

        computeBounds(min, max, r, b, mapSize);
        Point[] wraps = new Point[(max.x - min.x + 1) * (max.y - min.y + 1)];
        int i = 0;
        for (int xoff = min.x; xoff <= max.x; xoff++) {
            for (int yoff = min.y; yoff <= max.y; yoff++) {
                wraps[i++] =
                    new Point(
                        xoff * mapSize.width + p.x,
                        yoff * mapSize.height + p.y);
            }
        }
        return wraps;
    }

    public boolean containsWrapped(Rectangle r, Point p) {
        return computeWraps(r, p).length > 0;
    }

    public boolean containsWrapped(MapObject o, Point p) {
        Point[] wraps = computeWraps(o.getBounds(), p);
        for (int i = 0; i < wraps.length; i++)
            if (o.contains(wraps[i]))
                return true;
        return false;
    }
    
    public AffineTransform getTransform() {
        if (at == null) {
            at = new AffineTransform();
            at.translate(
                -scale * model.options.getSize().width / 2
                    + getSize().width / 2
                    - offset.x,
                scale * model.options.getSize().height / 2
                    + getSize().height / 2
                    - offset.y);
            at.scale(scale, -scale);
        }
        return at;
    }

    public AffineTransform getInverse() {
        try {
            if (it == null)
                it = getTransform().createInverse();
            return it;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
    
    
    public void beginEdit() {
        currentEdit = new CompoundMapEdit();
    }
    
    public void commitEdit() {
        currentEdit.edit();
        getUndoManager().addEdit(currentEdit);
        repaint();
        currentEdit = null;
    }
    
    public void cancelEdit() {
        currentEdit = null;
    }

    private void doEdit(MapEdit e) {
        if (currentEdit != null) { 
            currentEdit.addEdit(e);
        } else {
            e.edit();
            getUndoManager().addEdit(e);
            repaint();
        }
    }
    
    private interface MapEdit extends UndoableEdit {
        public void edit();
        public void unedit();
    }
    
    private abstract class AbstractMapEdit 
    extends AbstractUndoableEdit implements MapEdit {
        public void redo() {
            super.redo();
            edit();
        }
        public void undo() {
            super.undo();
            unedit();
        }
    }
    
    private class CompoundMapEdit 
    extends CompoundEdit implements MapEdit {
        public void edit() {
            for (Iterator i = edits.iterator(); i.hasNext();)
                ((MapEdit)i.next()).edit();
        }
        public void unedit() {}
    }
    
    public void setPolygonEdgeStyle(
        final MapPolygon p,
        final int edgeIndex,
        final LineStyle newStyle) {
        final LineStyle oldStyle = p.getEdgeStyle(edgeIndex);
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                p.setEdgeStyle(edgeIndex, oldStyle);
            }
            public void edit() {
                p.setEdgeStyle(edgeIndex, newStyle);
            }
        });
    }

    public void removePolygonPoint(final MapPolygon p, final int i) {
        final Point point = p.getPoint(i);
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                p.insertPoint(i, point);
            }
            public void edit() {
                p.removePoint(i);
            }
        });
    }

    public void insertPolygonPoint(
        final MapPolygon p,
        final int i,
        final Point point) {
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                p.removePoint(i);
            }
            public void edit() {
                p.insertPoint(i, point);
            }
        });
    }

    public void movePolygonPoint(
        final MapPolygon p,
        final int i,
        final int newX,
        final int newY) {
        final Point oldPoint = p.getPoint(i);
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                p.setPoint(i, oldPoint.x, oldPoint.y);
            }
            public void edit() {
                p.setPoint(i, newX, newY);
            }
        });
    }

    public void setPolygonStyles(
        final MapPolygon p,
        final Map newStyles) {
        doEdit(new AbstractMapEdit() {
            Map oldStyles = p.getStyles();
            Map oldDefaultStyles = model.getDefaultPolygonStyles();
            public void unedit() {
                p.setStyles(oldStyles);
                model.setDefaultPolygonStyles(oldDefaultStyles);
            }
            public void edit() {
                p.setStyles(newStyles);
                model.setDefaultPolygonStyles(newStyles);
            }
        });
    }
    
    public void rotatePolygon(
        final MapPolygon p,
        final double angle) {
        final MapPolygon rotated = (MapPolygon)p.copy();
        rotated.rotate(angle);
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                getModel().replace(rotated, p);
            }
            public void edit() {
                getModel().replace(p, rotated);            
            }
        });
    }

    public void setObjectTeam(final MapObject o, final int newTeam) {
        final int oldTeam = o.getTeam();
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                o.setTeam(oldTeam);
            }
            public void edit() {
                o.setTeam(newTeam);
            }
        });
    }

    public void setBaseProperties(
        final Base b,
        final int newTeam,
        final int newDir,
        final int newOrder) {
        final int oldTeam = b.getTeam();
        final int oldDir = b.getDir();
        final int oldOrder = b.getOrder();
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                b.setTeam(oldTeam);
                b.setDir(oldDir);
                b.setOrder(oldOrder);
            }
            public void edit() {
                b.setTeam(newTeam);
                b.setDir(newDir);
                b.setOrder(newOrder);
            }
        });
    }
    
    public void setBallProperties(
        final Ball b,
        final int newTeam,
        final PolygonStyle newStyle) {
        final int oldTeam = b.getTeam();
        final PolygonStyle oldStyle = b.getStyle();
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                b.setTeam(oldTeam);
                b.setStyle(oldStyle);                
            }
            public void edit() {
                b.setTeam(newTeam);
                b.setStyle(newStyle);
            }
        });
    }
    
    public void setGravProperties(
        final Grav grav,
        final int newType,
        final BigDecimal newForce) {
        final int oldType = grav.getType();
        final BigDecimal oldForce = grav.getForce();
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                grav.setType(oldType);
                grav.setForce(oldForce);
            }
            public void edit() {
                grav.setType(newType);
                grav.setForce(newForce);
            }
        });
    }
    
    public void setWormholeType(final Wormhole o, final int newType) {
        final int oldType = o.getType();
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                o.setType(oldType);
            }
            public void edit() {
                o.setType(newType);
            }            
        });
    }
    
    public void setWormholePoint(final Wormhole o, final int x, final int y) {
        doEdit(new AbstractMapEdit() {
            private Point oldPoint = o.getPoint();
            public void unedit() {
                o.setPoint(oldPoint);
            }
            public void edit() {
                o.setPoint(new Point(x, y));
            }
        });
    }

    public void setCannonDir(final Cannon c, final int newDir) {
        final int oldDir = c.getDir();
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                c.setDir(oldDir);
            }
            public void edit() {
                c.setDir(newDir);
            }            
        });        
    }
    
    public void setCannonPoint(final Cannon c, final int x, final int y) {
        final Point oldPoint = c.getPoint();
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                c.setPoint(oldPoint);
            }
            public void edit() {
                c.setPoint(new Point(x, y));
            }            
        });        
    }
    
    public void removeMapObject(final MapObject o) {
        final int i = getModel().indexOf(o);
        if (i == -1)
            return;
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                getModel().addObject(i, o);
            }
            public void edit() {
                getModel().removeObject(i);
            }
        });
    }

    public void addMapObject(final MapObject o) {
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                getModel().removeObject(o);
            }
            public void edit() {
                getModel().addToFront(o);
            }
        });
    }
    
    public void bringMapObject(final MapObject o, final boolean front) {
        final int i = getModel().indexOf(o);
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                getModel().removeObject(o);
                getModel().addObject(i, o);
            }
            public void edit() {
                getModel().removeObject(o);
                getModel().addObject(o, front);
            }
        });
    }
        
    public void makeBallAreaFromSelected() {
        if (selected.isEmpty()) return;
        makeGroup(new BallArea(selected));
    }
    
    public void makeBallTargetFromSelected() {
        if (selected.isEmpty()) return;
        makeGroup(new BallTarget(selected));
    }
    
    public void makeCannonFromSelected() {
        if (selected.isEmpty()) return;
        makeGroup(new Cannon(selected));
    }    

    public void makeDecorationFromSelected() {
        if (selected.isEmpty()) return;
        makeGroup(new Decoration(selected));
    }
    
    public void makeFrictionAreaFromSelected() {
        if (selected.isEmpty()) return;
        makeGroup(new FrictionArea(selected));        
    }
    
    public void makeGroupFromSelected() {
        if (selected.isEmpty()) return;
        makeGroup(new Group(selected));
    }    
    
    public void makeTargetFromSelected() {
        if (selected.isEmpty()) return;
        makeGroup(new Target(selected));        
    }    
    
    public void makeWormholeFromSelected() {
        if (selected.isEmpty()) return;
        makeGroup(new Wormhole(selected));        
    }    

    private void makeGroup(final Group g) {
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                for (Iterator i = g.getMembers().iterator(); i.hasNext();) {
                    MapObject o = (MapObject)i.next();
                    o.setParent(null);
                    getModel().addToFront(o);
                }
            }
            public void edit() {
                for (Iterator i = selected.iterator(); i.hasNext();) {
                    MapObject o = (MapObject)i.next();
                    o.setParent(g);
                    getModel().removeObject(o);
                }
                getModel().addToFront(g);
                setSelectedObject(g);
            }            
        });
    }
        
    public void ungroupSelected() {
        if (selected.isEmpty() 
            || !(selected.get(0) instanceof Group))
            return;
        doEdit(new AbstractMapEdit() {
            Group g = (Group)selected.get(0);
            public void unedit() {
                getModel().addToFront(g);
                for (Iterator i = g.getMembers().iterator(); i.hasNext();)
                    getModel().removeObject((MapObject)i.next());                
            }
            public void edit() {
                getModel().removeObject(g);
                setSelectedObject(null);
                for (Iterator i = g.getMembers().iterator(); i.hasNext();) {
                    MapObject o = (MapObject)i.next();
                    getModel().addToFront(o);
                    addSelectedObject(o);
                }
            }
        });
    }
    
    public void regroupSelected() {
        if (selected.isEmpty() 
            || ((MapObject)selected.get(0)).getParent() == null)
            return;
        doEdit(new AbstractMapEdit() {
            Group g = ((MapObject)selected.get(0)).getParent();
            public void unedit() {
                getModel().removeObject(g);
                for (Iterator i = g.getMembers().iterator(); i.hasNext();)
                    getModel().addToFront((MapObject)i.next());                    
            }
            public void edit() {
                getModel().addToFront(g);
                for (Iterator i = g.getMembers().iterator(); i.hasNext();)
                    getModel().removeObject((MapObject)i.next());
                setSelectedObject(g);                
            }
        });
    }

    public void moveMapObject(
        final MapObject o,
        final int newX,
        final int newY) {
        final int oldX = o.getBounds().x;
        final int oldY = o.getBounds().y;
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                o.moveTo(oldX, oldY);
            }
            public void edit() {
                o.moveTo(newX, newY);
            }
        });
    }

    public void addEdgeStyle(final LineStyle style) {
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                getModel().edgeStyles.remove(style);
            }
            public void edit() {
                getModel().edgeStyles.add(style);
            }
        });
    }

    public void removeEdgeStyle(final LineStyle style) {
        final java.util.List oldStyles = getModel().edgeStyles;
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                getModel().edgeStyles = oldStyles;
            }
            public void edit() {
                getModel().edgeStyles.remove(style);
            }
        });
    }

    public void addPolygonStyle(final PolygonStyle style) {
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                getModel().polyStyles.remove(style);
            }
            public void edit() {
                getModel().polyStyles.add(style);
            }
        });
    }

    public void removePolygonStyle(final PolygonStyle style) {
        final java.util.List oldStyles = new ArrayList(getModel().polyStyles);
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                getModel().polyStyles = oldStyles;
            }
            public void edit() {
                getModel().polyStyles.remove(style);
            }
        });
    }

    public void setEdgeStyleProperties(
        final LineStyle style,
        final String newId,
        final int newStyle,
        final int newWidth,
        final Color newColor) {
        final String oldId = style.getId();
        final int oldStyle = style.getStyle();
        final int oldWidth = style.getWidth();
        final Color oldColor = style.getColor();
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                set(oldId, oldStyle, oldWidth, oldColor);
            }
            public void edit() {
                set(newId, newStyle, newWidth, newColor);
            }
            private void set(String id, int s, int w, Color c) {
                style.setId(id);
                style.setStyle(s);
                style.setWidth(w);
                style.setColor(c);
            }
        });
    }

    public void setPolygonStyleProperties(
        final PolygonStyle style,
        final String newId,
        final int newFillStyle,
        final Color newColor,
        final Pixmap newTexture,
        final LineStyle newDefEdgeStyle,
        final boolean newVisible,
        final boolean newVisInRadar) {
        final String oldId = style.getId();
        final int oldFillStyle = style.getFillStyle();
        final Color oldColor = style.getColor();
        final Pixmap oldTexture = style.getTexture();
        final LineStyle oldDefEdgeStyle = style.getDefaultEdgeStyle();
        final boolean oldVisible = style.isVisible();
        final boolean oldVisInRadar = style.isVisibleInRadar();
        doEdit(new AbstractMapEdit() {
            public void unedit() {
                set(
                    oldId,
                    oldFillStyle,
                    oldColor,
                    oldTexture,
                    oldDefEdgeStyle,
                    oldVisible,
                    oldVisInRadar);
            }
            public void edit() {
                set(
                    newId,
                    newFillStyle,
                    newColor,
                    newTexture,
                    newDefEdgeStyle,
                    newVisible,
                    newVisInRadar);
            }
            private void set(
                String id,
                int fs,
                Color c,
                Pixmap t,
                LineStyle es,
                boolean v,
                boolean vr) {
                style.setId(id);
                style.setFillStyle(fs);
                style.setColor(c);
                style.setTexture(t);
                style.setDefaultEdgeStyle(es);
                model.setDefaultEdgeStyle(es);
                style.setVisible(v);
                style.setVisibleInRadar(vr);
            }
        });
    }

    private class AwtEventHandler implements CanvasEventHandler {
        
        public void mouseClicked(MouseEvent evt) {
            if (model == null)
                return;
            transformEvent(evt, true);
            if (eventHandler != null) {
                eventHandler.mouseClicked(evt);
                return;
            }
            for (Iterator iter = model.objects.iterator(); iter.hasNext();) {
                MapObject o = (MapObject) iter.next();
                if (o.checkAwtEvent(MapCanvas.this, evt)) {
                    return;
                }
            }
        }

        public void mouseEntered(MouseEvent evt) {
            if (model == null)
                return;
            if (eventHandler != null) {
                transformEvent(evt, true);
                eventHandler.mouseEntered(evt);
                return;
            }
        }

        public void mouseExited(MouseEvent evt) {
            if (model == null)
                return;
            if (eventHandler != null) {
                transformEvent(evt, true);
                eventHandler.mouseExited(evt);
                return;
            }
        }

        public void mousePressed(MouseEvent evt) {
            if (model == null)
                return;
            transformEvent(evt, true);
            if (eventHandler != null) {
                eventHandler.mousePressed(evt);
                return;
            }
            for (Iterator iter = model.objects.iterator(); iter.hasNext();) {
                MapObject o = (MapObject) iter.next();
                if (o.checkAwtEvent(MapCanvas.this, evt)) {
                    return;
                }
            }
            if ((evt.getModifiers() & InputEvent.BUTTON1_MASK) != 0) {
                if (evt.isControlDown()) {            
                    setSelectedObject(null);
                    setCanvasEventHandler(selectHandler);
                    selectHandler.mousePressed(evt);
                } else {
                    dragHandler.mousePressed(evt);
                    setCanvasEventHandler(dragHandler);
                    setSelectedObject(null);
                }
            }
        }

        public void mouseReleased(MouseEvent evt) {           
            if (model == null)
                return;
            transformEvent(evt, true);
            if (eventHandler != null) {
                eventHandler.mouseReleased(evt);
                return;
            }
            for (Iterator iter = model.objects.iterator(); iter.hasNext();) {
                MapObject o = (MapObject) iter.next();
                if (o.checkAwtEvent(MapCanvas.this, evt)) {
                    return;
                }
            }
        }

        public void mouseDragged(MouseEvent evt) {
            if (model == null)
                return;
            transformEvent(evt, true);
            if (eventHandler != null) {
                eventHandler.mouseDragged(evt);
                return;
            }
        }

        public void mouseMoved(MouseEvent evt) {
            if (model == null)
                return;
            if (eventHandler != null) {
                transformEvent(evt, true);
                eventHandler.mouseMoved(evt);
                return;
            }
        }

        private void transformEvent(MouseEvent evt, boolean snap) {
            int x = evt.getX();
            int y = evt.getY();
            int g = grid * 64;
            Point mapp = new Point(x, y);
            getInverse().transform(mapp, mapp);
            if (snap && g > 0) {
                mapp.x = (mapp.x / g) * g;
                mapp.y = (mapp.y / g) * g;
            }
            evt.translatePoint(mapp.x - evt.getX(), mapp.y - evt.getY());
        }        
    }
    
    private class DragHandler extends CanvasEventAdapter {
        
        private Point dragStart;
        private Point offsetStart;
        private boolean wasFastRendering;        
        
        public void mousePressed(MouseEvent evt) {
            toScreen(evt);
            dragStart = evt.getPoint();
            offsetStart = new Point(offset);
            wasFastRendering = isFastRendering();
            setFastRendering(true);           
        }
        
        public void mouseDragged(MouseEvent evt) {
            toScreen(evt);
            at = null;
            it = null;
            offset.x = offsetStart.x + (dragStart.x - evt.getX());
            offset.y = offsetStart.y + (dragStart.y - evt.getY());
            repaint();           
        }
        
        public void mouseReleased(MouseEvent evt) {
            setFastRendering(wasFastRendering);
            setCanvasEventHandler(null);
            repaint();                        
        }
        
        private void toScreen(MouseEvent evt) {
            Point mapp = new Point();
            Point evtp = evt.getPoint();
            getTransform().transform(evtp, mapp);
            evt.translatePoint(mapp.x - evtp.x, mapp.y - evtp.y);
        }                
    }
    
    private class SelectHandler extends CanvasEventAdapter {
        
        private Point selectStart;
        private Rectangle selectShape = new Rectangle();
        
        public void mousePressed(MouseEvent evt) {
            if ((evt.getModifiers() 
            & InputEvent.BUTTON1_MASK) != 0) {
                selectStart = evt.getPoint();
                paintSelectArea(evt, false, true);
            }
        }
        
        public void mouseDragged(MouseEvent evt) {
            if ((evt.getModifiers() 
            & InputEvent.BUTTON1_MASK) != 0)
                paintSelectArea(evt, true, true);
        }
        
        public void mouseReleased(MouseEvent evt) {
            if ((evt.getModifiers()
            & InputEvent.BUTTON1_MASK) != 0) {            
                paintSelectArea(evt, true, false);
                setCanvasEventHandler(null);
                repaint();
            }
        }           
        
        private void paintSelectArea(
        MouseEvent evt, boolean erase, boolean paint) {
            Graphics2D g = (Graphics2D)getGraphics();
            g.setXORMode(Color.black);
            g.setColor(Color.white);
            if (erase) drawShape(g, selectShape);
            selectShape.setBounds(
                Math.min(selectStart.x, evt.getX()),
                Math.min(selectStart.y, evt.getY()),
                Math.abs(selectStart.x - evt.getX()),
                Math.abs(selectStart.y - evt.getY()));
            if (paint) drawShape(g, selectShape);
        }
    }
}
