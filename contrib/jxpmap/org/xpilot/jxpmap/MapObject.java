package org.xpilot.jxpmap;

import java.awt.AWTEvent;
import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.Shape;
import java.awt.Stroke;
import java.awt.event.MouseEvent;
import java.awt.geom.AffineTransform;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Map;
import java.util.Comparator;

import javax.swing.ImageIcon;


public abstract class MapObject extends ModelObject {
    
    public static final Stroke SELECTED_STROKE = new BasicStroke(1);    
    
    public static final Comparator Z_ORDER = new Comparator() {
        public int compare(Object o1, Object o2) {
            return ((MapObject)o1).getZOrder() - ((MapObject)o2).getZOrder();
        }
    };
    
    protected Rectangle bounds;
    protected Stroke previewStroke;
    protected Image img;
    protected MapObjectPopup popup;
    protected int team;
    protected boolean selected;
    protected Group parent;
    
    public Object deepClone (Map context) {
        MapObject clone = (MapObject)super.deepClone(context);
        clone.bounds = new Rectangle(bounds);
        clone.popup = null;
        return clone;
    }
    
    public MapObject copy() {
        MapObject copy = (MapObject)clone();
        copy.bounds = new Rectangle(bounds);
        copy.popup = null;
        return copy;
    }
    
    public MapObject () {
        this(null, 0, 0, 0, 0);
    }


    public MapObject (int width, int height) {
        this(null, 0, 0, width, height);
    }


    public MapObject (String imgName, int width, int height) {
        this(imgName, 0, 0, width, height);
    }


    public MapObject (String imgName, int x, int y, int width, int height) {
        bounds = new Rectangle(x, y, width, height);
        if (imgName != null) setImage(imgName);
    }
    
    
    public MapObject newInstance() {
        try {
            return (MapObject)getClass().newInstance();
        } catch (Exception e) {
            throw new RuntimeException("MapObject creation failed: " + e);
        } 
    }

    public boolean isSelected() {
        return selected;
    }
    
    public void setSelected(boolean value) {
        this.selected = value;
    }

    public int getTeam () {
        return team;
    }
    
    public void setTeam (int team) {
        this.team = team;
    }
    
    public Group getParent () {
        return parent;
    }
    
    public void setParent (Group parent) {
        this.parent = parent;
    }

    public Rectangle getBounds () {
        return bounds;
    }


    public void setBounds (Rectangle bounds) {
        this.bounds = bounds;
    }


    public boolean contains (Point p) {
        return getBounds().contains(p);
    }

    public int getZOrder () {
        return 10;
    }


    protected Image getImage () {
        return img;
    }


    protected void setImage (Image img) {
        this.img = img;
    }


    protected void setImage (String fileName) {
        this.img = 
            (new ImageIcon
                (getClass().getResource
                 ("/images/" + fileName))).getImage();
    }


    protected Shape getPreviewShape () {
        return getBounds();
    }

    private MapObjectPopup getPopup() {
        if (popup == null) popup = createPopup();
        return popup;
    }
    
    protected MapObjectPopup createPopup() {
        return new MapObjectPopup(this);
    }

    protected Stroke getPreviewStroke (float scale) {
        if (previewStroke == null) {
            float dash[] = { 10 / scale };
            previewStroke = 
                new BasicStroke
                    (1, BasicStroke.CAP_BUTT, 
                     BasicStroke.JOIN_MITER, 
                     10 / scale, dash, 0.0f);
        }
        return previewStroke;
    }


    public CanvasEventHandler getCreateHandler (Runnable r) {
        return new CreateHandler(r);
    }


    public EditorPanel getPropertyEditor (MapCanvas canvas) {
        EditorPanel editor = new EditorPanel();
        editor.add(new javax.swing.JLabel("No editable properties"));
        return editor;
    }


    public boolean checkAwtEvent (MapCanvas canvas, AWTEvent evt) {

        if (evt instanceof MouseEvent) {
            MouseEvent me = (MouseEvent)evt;

            if (canvas.containsWrapped(this, me.getPoint())) {
                if (me.isPopupTrigger()) {
                    Point p = me.getPoint();
                    canvas.getTransform().transform(p, p);
                    getPopup().show(canvas, p.x, p.y);
                    return true;

                } else {
                    if (evt.getID() == MouseEvent.MOUSE_PRESSED
                    && (me.getModifiers() & MouseEvent.BUTTON1_MASK) != 0) {
                        if (canvas.getMode() == MapCanvas.MODE_ERASE) {
                            canvas.removeMapObject(this);
                        } else if (canvas.getMode() == MapCanvas.MODE_COPY) {
                            canvas.setCanvasEventHandler(
                                copy().new CopyHandler(me));
                        } else {
                            if ((me.getModifiersEx() 
                            & MouseEvent.CTRL_DOWN_MASK) != 0) {
                                canvas.addSelectedObject(this);
                            } else {
                                canvas.setSelectedObject(this);
                            }
                            canvas.setCanvasEventHandler(new MoveHandler(me));
                        }
                    }
                    return true;            
                }
            }
        }
        return false;
    }


    public void moveTo (int x, int y) {
        bounds.x = x;
        bounds.y = y;
    }
    

    public void paint (Graphics2D g, float scale) {
        if (img != null) {
            Rectangle r = getBounds();
            AffineTransform at = AffineTransform.getTranslateInstance
                (r.x, r.y + r.height);
            at.scale(64, -64);
            g.drawImage(img, at, null);
        }
        if (isSelected()) {
            g.setColor(Color.white);
            g.draw(getBounds());
        }
    }


    public abstract void printXml (PrintWriter out) throws IOException;



    //---------------------------------------------------------------------
    // Inner classes

    

    protected class CreateHandler extends CanvasEventAdapter {
        
        private Runnable cmd;
        private Point offset;
        private Shape preview;
        private Shape toBeRemoved;
        private Stroke stroke;


        public CreateHandler (Runnable cmd) {
            this.cmd = cmd;
            Rectangle ob = MapObject.this.getBounds();
            offset = new Point(ob.width / 2, ob.height / 2);
            preview = getPreviewShape();
        }


        public void mouseMoved (MouseEvent evt) {

            Rectangle ob = MapObject.this.getBounds();
            MapCanvas c = (MapCanvas)evt.getSource();
            
            Graphics2D g = (Graphics2D)c.getGraphics();
            g.setXORMode(Color.black);
            g.setColor(Color.white);
            if (stroke == null) stroke = getPreviewStroke(c.getScale());
            //g.setStroke(stroke);
            
            if (toBeRemoved != null) c.drawShape(g, toBeRemoved);
            
            AffineTransform tx = AffineTransform.getTranslateInstance
                (evt.getX() - offset.x - ob.x, evt.getY() - offset.y - ob.y);
            c.drawShape(g, toBeRemoved = tx.createTransformedShape(preview));
        }


        public void mousePressed (MouseEvent evt) {

            MapCanvas c = (MapCanvas)evt.getSource();
            MapObject.this.moveTo(evt.getX() - offset.x, 
                                  evt.getY() - offset.y);
            c.addMapObject(MapObject.this);
            c.setCanvasEventHandler(newInstance().getCreateHandler(cmd));
            if (cmd != null) cmd.run();
        }
    }


    
    protected class MoveHandler extends CanvasEventAdapter {


        protected Point offset;
        protected Shape preview;
        protected Shape toBeRemoved;
        protected Stroke stroke;


        public MoveHandler (MouseEvent evt) {
            Rectangle ob = MapObject.this.getBounds();
            offset = new Point(evt.getX() - ob.x, evt.getY() - ob.y);
            preview = getPreviewShape();
        }


        public void mouseDragged (MouseEvent evt) {

            Rectangle ob = MapObject.this.getBounds();
            MapCanvas c = (MapCanvas)evt.getSource();

            Graphics2D g = (Graphics2D)c.getGraphics();
            g.setXORMode(Color.black);
            g.setColor(Color.white);
            if (stroke == null) stroke = getPreviewStroke(c.getScale());
            //g.setStroke(stroke);

            if (toBeRemoved != null) c.drawShape(g, toBeRemoved);

            AffineTransform tx = AffineTransform.getTranslateInstance
                (evt.getX() - offset.x - ob.x, evt.getY() - offset.y - ob.y);
            c.drawShape(g, toBeRemoved = tx.createTransformedShape(preview));
        }

        
        public void mouseReleased (MouseEvent evt) {
            MapCanvas c = (MapCanvas)evt.getSource();
            c.moveMapObject(MapObject.this,
                            evt.getX() - offset.x, 
                            evt.getY() - offset.y);
            c.setCanvasEventHandler(null);
        }
    }
    
    protected class CopyHandler extends MoveHandler {
        
        public CopyHandler(MouseEvent me) {
            super(me);
        }
        
        public void mouseReleased (MouseEvent evt) {
            MapCanvas c = (MapCanvas)evt.getSource();
            MapObject.this.moveTo(
                evt.getX() - offset.x, 
                evt.getY() - offset.y);
            c.addMapObject(MapObject.this);
            c.setCanvasEventHandler(null);
        }        
    }
}
