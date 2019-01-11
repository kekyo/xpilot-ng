/*
 * $Id: Wormhole.java,v 1.4 2005/03/12 17:11:58 juhal Exp $
 */
package org.xpilot.jxpmap;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.geom.Point2D;
import java.awt.geom.AffineTransform;
import java.awt.AWTEvent;
import java.awt.event.MouseEvent;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Collection;

import javax.swing.JComboBox;
import javax.swing.JLabel;

/**
 * @author jli
 */
public class Wormhole extends Group {
    
    public static final int TYPE_NORMAL = 0;
    public static final int TYPE_IN = 1;
    public static final int TYPE_OUT = 2;
    public static final int TYPE_FIXED = 3;

    public static final String[] TYPES = {
        "normal", "in", "out", "fixed"        
    };

    private int x, y, type;
    private AffineTransform at;
    private Rectangle pointArea;


    public Wormhole() {
        super();
        setType(TYPE_NORMAL);
    }
    
    public Wormhole(Collection c) {
        super(c);
        this.x = (int)getBounds().getCenterX();
        this.y = (int)getBounds().getCenterY();
        setType(TYPE_NORMAL);
    }
    
    public Wormhole(Collection c, int x, int y, int type) {
        super(c);
        this.x = x;
        this.y = y;
        setType(type);
    }
    
    public Wormhole (Collection c, int x, int y, String typeStr) {
        super(c);
        this.x = x;
        this.y = y;
        setType(TYPE_NORMAL);        
        for (int i = 0; i < TYPES.length; i++) {
            if (typeStr.equals(TYPES[i])) {
                setType(type);
                break;
            }
        }
    }
    
    public void moveTo(int x, int y) {
        Rectangle r = getBounds();
        this.x += x - r.x;
        this.y += y - r.y;
        this.at = null;
        this.pointArea = null;
        super.moveTo(x, y);
    }    
    
    public Point getPoint() {
        return new Point(x, y);
    }
    
    public void setPoint(Point p) {
        setPoint(p.x, p.y);
    }
    
    public void setPoint(int x, int y) {
        this.x = x;
        this.y = y;
        this.at = null;
        this.pointArea = null;        
    }

    public int getType() {
        return type;
    }

    public void setType(int i) {
        if (i >= TYPES.length) 
            throw new IllegalArgumentException("illegal wormhole type: " + i);
        type = i;
        setImage("wormhole_" + TYPES[i] + ".gif");
    }
    
    public void setType(String name) {
        for (int i = 0; i < TYPES.length; i++) {
            if (TYPES[i].equalsIgnoreCase(name)) {
                setType(i);
                return;
            }
        }
        throw new IllegalArgumentException("illegal wormhole type: " + name);
    }
    
    protected AffineTransform getImageTransform() {
        if (at == null) {
            at = AffineTransform.getTranslateInstance(x, y);
            at.scale(64, -64);
        }
        return at;
    }
    
    protected Rectangle getPointArea() {
        if (pointArea == null) {
            int w = 64 * getImage().getWidth(null);
            int h = 64 * getImage().getHeight(null); 
            pointArea = new Rectangle(x, y - h, w, h);
        }
        return pointArea;
    }
    
    public void paint(Graphics2D g, float scale) {
        super.paint(g, scale);
        if (type != TYPE_IN)
            g.drawImage(getImage(), getImageTransform(), null);
        if (type == TYPE_FIXED && isSelected()) {
            g.setColor(Color.white);
            g.setStroke(getPreviewStroke(scale));
            g.drawLine(
                (int)getBounds().getBounds2D().getCenterX(),
                (int)getBounds().getBounds2D().getCenterY(),
                x, y);
        }
    }    
    
    public void printXml (PrintWriter out) throws IOException {
        out.print("<Wormhole x=\"");
        out.print(x);
        out.print("\" y=\"");
        out.print(y);
        out.print("\" type=\"");
        out.print(TYPES[getType()]);
        out.println("\">");
        super.printMemberXml(out);
        out.println("</Wormhole>");
    }

    
    public EditorPanel getPropertyEditor (MapCanvas c) {
        return new WormholePropertyEditor(c);
    }


    private class WormholePropertyEditor extends EditorPanel {

        private JComboBox cmbType;
        private MapCanvas canvas;


        public WormholePropertyEditor (MapCanvas canvas) {

            setTitle("Wormhole");

            cmbType = new JComboBox();
            for (int i = 0; i < TYPES.length; i++) 
                cmbType.addItem(TYPES[i]);
            cmbType.setSelectedIndex(getType());
            add(new JLabel("Type"));
            add(cmbType);
            
            this.canvas = canvas;
        }
        
        
        public boolean apply () {
            int newType = cmbType.getSelectedIndex() ;
            canvas.setWormholeType(Wormhole.this, newType);
            return true;
        }
    }

    
    public boolean checkAwtEvent(MapCanvas canvas, AWTEvent evt) {
        if (canvas.getMode() == MapCanvas.MODE_EDIT 
        && evt.getID() == MouseEvent.MOUSE_PRESSED) {
            MouseEvent me = (MouseEvent)evt;
            if (
            (me.getModifiers() & MouseEvent.BUTTON1_MASK) != 0) {
                if (getPointArea().contains(me.getX(), me.getY())) {
                    canvas.setCanvasEventHandler(
                        new PointMoveHandler(me));
                    return true;
                }
            }
        }
        return super.checkAwtEvent(canvas, evt);
    }
    
    protected class PointMoveHandler extends CanvasEventAdapter {
        
        private Point dragStart;
        private boolean clear;
        private int nx, ny;
        private AffineTransform at;
        
        public PointMoveHandler(MouseEvent evt) {
            dragStart = evt.getPoint();
            at = new AffineTransform();
        }
        public void mouseReleased(MouseEvent evt) {
            MapCanvas c = (MapCanvas)evt.getSource();
            c.setCanvasEventHandler(null);
            c.setWormholePoint(Wormhole.this, nx, ny);
            c.repaint();
        }
        public void mouseDragged(MouseEvent evt) {
            MapCanvas c = (MapCanvas)evt.getSource();
            Graphics2D g = (Graphics2D)c.getGraphics();
            g.setXORMode(Color.black);
            if (clear) drawPreviewImage(g);
            updatePoint(evt);
            drawPreviewImage(g);
            clear = true;
        }
        private void updatePoint(MouseEvent evt) {
            nx = x + evt.getX() - dragStart.x;
            ny = y + evt.getY() - dragStart.y;
            at.setToIdentity();
            at.concatenate(((MapCanvas)evt.getSource()).getTransform());            
            at.translate(nx, ny);
            at.scale(64, -64);
        }
        private void drawPreviewImage(Graphics2D g) {
            g.drawImage(getImage(), at, null);
        }
    }
}
