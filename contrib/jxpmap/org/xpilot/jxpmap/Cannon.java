package org.xpilot.jxpmap;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.Polygon;
import java.awt.Rectangle;
import java.awt.Shape;
import java.awt.geom.Point2D;
import java.awt.geom.AffineTransform;
import java.awt.geom.GeneralPath;
import java.awt.AWTEvent;
import java.awt.event.MouseEvent;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Collection;
import java.util.Map;

public class Cannon extends Group {
    
    private static final GeneralPath ARROW = new GeneralPath();
    static {
        ARROW.moveTo(0, 0);
        ARROW.lineTo(27, 0);
        ARROW.lineTo(27, 3);
        ARROW.lineTo(30, 0);
        ARROW.lineTo(27, -3);
        ARROW.lineTo(27, 0);
    }
    private static final Point HEAD = new Point(30, 0);

    private int x, y, dir;
    private Shape arrow;
    private Point2D head;
    
    public MapObject copy() {
        Cannon copy = (Cannon)super.copy();
        copy.arrow = new GeneralPath(arrow);
        return copy;
    }

    public Object deepClone(Map context) {
        Cannon clone = (Cannon)super.deepClone(context);
        clone.arrow = new GeneralPath(arrow);
        return clone;
    }
    
    public Cannon() {
        super();
        setTeam(-1);
        updateArrow();
    }
    
    public Cannon(Collection c) {
        super(c);
        x = (int)getBounds().getCenterX();
        y = (int)getBounds().getCenterY();
        updateArrow();
    }

    public Cannon(Collection c, int team, int x, int y, int dir) {
        super(c);
        setTeam(team);
        this.dir = dir;
        this.x = x;
        this.y = y;
        updateArrow();
    }


    public int getDir() {
        return dir;
    }


    public void setDir(int dir) {
        if (dir > 127) dir = 127;
        if (dir < 0) dir = 0;
        this.dir = dir;
        updateArrow();
    }
    
    
    public void setPoint(Point p) {
        setPoint(p.x, p.y);
    }
    
    public void setPoint(int x, int y) {
        this.x = x;
        this.y = y;
        updateArrow();
    }

    
    
    public Point getPoint() {
        return new Point(x, y);
    }
    

    public void moveTo(int x, int y) {
        Rectangle r = getBounds();
        this.x += x - r.x;
        this.y += y - r.y;
        updateArrow();        
        super.moveTo(x, y);
    }
    
    protected void updateArrow() {
        AffineTransform at = 
            computeTransform(x, y, 2 * Math.PI * getDir() / 128.0);
        arrow = at.createTransformedShape(ARROW);
        head = at.transform(HEAD, new Point());
    }
        
    protected AffineTransform computeTransform(int x, int y, double a) {
        AffineTransform at = new AffineTransform();
        at.translate(x, y);
        at.rotate(a);
        at.scale(64, 64);
        return at;
    }

    public int getTeam() {
        return team;
    }


    public void setTeam(int team) {
        if (team < -1 || team > 10)
            throw new IllegalArgumentException
                ("illegal team: " + team);
        this.team = team;
    }

    
    public void printXml(PrintWriter out) throws IOException {
        out.println("<Cannon x=\"" + x + "\" y=\"" + y 
            + "\" dir=\"" + dir + "\" team=\"" + team + "\">");
        super.printMemberXml(out);
        out.println("</Cannon>");
    }

    
    public EditorPanel getPropertyEditor(MapCanvas c) {
        return new TeamEditor("Cannon", c, this);
    }

    public void paint(Graphics2D g, float scale) {
        super.paint(g, scale);
        g.setColor(Color.yellow);
        g.draw(arrow);
    }
    
    public boolean checkAwtEvent(MapCanvas canvas, AWTEvent evt) {
        if (canvas.getMode() == MapCanvas.MODE_EDIT 
        && evt.getID() == MouseEvent.MOUSE_PRESSED) {
            MouseEvent me = (MouseEvent)evt;
            if (
            (me.getModifiers() & MouseEvent.BUTTON1_MASK) != 0) {
                double thSq = 100 / (canvas.getScale() * canvas.getScale());
                if (Point2D.distanceSq(x, y, me.getX(), me.getY()) < thSq) {
                    canvas.setCanvasEventHandler(
                        new ArrowMoveHandler(me));
                    return true;
                } else if (Point2D.distanceSq(head.getX(), head.getY(), 
                    me.getX(), me.getY()) < thSq) {
                    canvas.setCanvasEventHandler(
                        new ArrowRotateHandler(me));
                    return true;
                }
            }
        }
        return super.checkAwtEvent(canvas, evt);
    }
    
    protected class ArrowMoveHandler extends CanvasEventAdapter {
        
        private Shape preview;
        private double angle;
        private boolean clear;
        
        public ArrowMoveHandler(MouseEvent evt) {
            preview = new GeneralPath(Cannon.this.arrow);
            angle = 2 * Math.PI * getDir() / 128.0;
        }
        public void mouseReleased(MouseEvent evt) {
            MapCanvas c = (MapCanvas)evt.getSource();
            c.setCanvasEventHandler(null);
            c.setCannonPoint(Cannon.this, evt.getX(), evt.getY());
            c.repaint();            
        }
        public void mouseDragged(MouseEvent evt) {
            MapCanvas c = (MapCanvas)evt.getSource();
            Graphics2D g = (Graphics2D)c.getGraphics();
            g.setColor(Color.white);
            g.setXORMode(Color.black);
            if (clear) c.drawShape(g, preview);
            preview = computeTransform(evt.getX(), evt.getY(), angle)
                .createTransformedShape(ARROW);                     
            c.drawShape(g, preview);
            clear = true;            
        }                
    }
    
    protected class ArrowRotateHandler extends CanvasEventAdapter {
        
        private Shape preview;
        private boolean clear;
        private double angle;

        public ArrowRotateHandler(MouseEvent evt) {
            preview = new GeneralPath(Cannon.this.arrow);
        }        
        public void mouseReleased(MouseEvent evt) {
            MapCanvas c = (MapCanvas)evt.getSource();
            c.setCanvasEventHandler(null);
            if (angle < 0) angle += 2 * Math.PI;
            c.setCannonDir(Cannon.this, (int)(64 * angle / Math.PI));
            c.repaint();
        }
        public void mouseDragged(MouseEvent evt) {
            MapCanvas c = (MapCanvas)evt.getSource();
            Graphics2D g = (Graphics2D)c.getGraphics();
            g.setColor(Color.white);
            g.setXORMode(Color.black);
            if (clear) c.drawShape(g, preview);
            angle = Math.atan2(evt.getY() - y, evt.getX() - x);
            preview = computeTransform(x, y, angle)
                .createTransformedShape(ARROW);            
            c.drawShape(g, preview);
            clear = true;
        }
    }
}
