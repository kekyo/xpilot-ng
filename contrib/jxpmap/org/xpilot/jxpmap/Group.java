package org.xpilot.jxpmap;

import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.ListIterator;
import java.io.PrintWriter;
import java.io.IOException;
import java.awt.Rectangle;
import java.awt.Point;
import java.awt.Graphics2D;
import java.awt.Shape;
import java.awt.Color;
import java.awt.geom.GeneralPath;


public class Group extends MapObject {
    
    protected List members;
    protected GeneralPath previewShape;

    public Object deepClone (Map context) {
        Group clone = (Group)super.deepClone(context);
        clone.members = new ArrayList();
        for (Iterator i = members.iterator(); i.hasNext();)
            clone.members.add(((MapObject)i.next()).deepClone(context));
        return clone;
    }
    
    public MapObject copy() {
        Group copy = (Group)super.clone();
        copy.members = new ArrayList();
        for (Iterator i = members.iterator(); i.hasNext();)
            copy.members.add(((MapObject)i.next()).copy()); 
        return copy;
    }
    
    public Group() {
        members = new ArrayList();
    }
    
    public Group(Collection c) {
        members = new ArrayList(c);
        Collections.sort(members, MapObject.Z_ORDER);
        invalidate();
    }
    
    public int getZOrder() {
        if (members.isEmpty()) return 100;
        return ((MapObject)members.get(0)).getZOrder();
    }
    
    public void addMember(MapObject o) {
        members.add(o);
        invalidate();
    }
    
    public void invalidate() {
        previewShape = null;
        bounds = null;
    }
    
    public void addToFront(MapObject moNew) {
        invalidate();
        for (ListIterator iter = members.listIterator(); iter.hasNext();) {
            MapObject moOld = (MapObject)iter.next();
            int znew = moNew.getZOrder();
            int zold = moOld.getZOrder();
            
            if (znew <= zold) {
                iter.previous();
                iter.add(moNew);
                return;
            }
        }
        members.add(moNew);
    }    
    
    public void removeMember(MapObject o) {
        if (members.remove(o)) invalidate();
    }
    
    public boolean hasMember(MapObject o) {
        return members.contains(o);
    }
    
    public List getMembers() {
        return Collections.unmodifiableList(members);
    }
    
    public Rectangle getBounds() {
        if (bounds == null) {
            int x1, y1, x2, y2;
            x1 = y1 = Integer.MAX_VALUE;
            x2 = y2 = Integer.MIN_VALUE;
            for (Iterator i = members.iterator(); i.hasNext();) {
                Rectangle b = ((MapObject)i.next()).getBounds();
                if (b.x < x1) x1 = b.x;
                if (b.y < y1) y1 = b.y;
                if (b.x + b.width > x2) x2 = b.x + b.width;
                if (b.y + b.height > y2) y2 = b.y + b.height;
            }
            bounds = new Rectangle(x1, y1, (x2 - x1), (y2 - y1));
        }
        return bounds;
    }
        
    public void setBounds(Rectangle r) {
        throw new UnsupportedOperationException();
    }
    
    public boolean contains (Point p) {
        for (Iterator i = members.iterator(); i.hasNext();)
            if (((MapObject)i.next()).contains(p)) return true;
        return false;
    }
    
    public Shape getPreviewShape () {
        if (previewShape == null) { 
            previewShape = new GeneralPath();
            //previewShape.append(getBounds(), false);
            for (Iterator i = members.iterator(); i.hasNext();)
                previewShape.append(((MapObject)i.next()).getPreviewShape(), 
                    false);
        }
        return previewShape;
    }
    
    public CanvasEventHandler getCreateHandler (Runnable r) {
        throw new UnsupportedOperationException();
    }
        
    public void moveTo (int x, int y) {
        Rectangle b = getBounds();
        int dx = x - b.x;
        int dy = y - b.y;
        for (Iterator i = members.iterator(); i.hasNext();) {
            MapObject o = (MapObject)i.next();
            b = o.getBounds();
            o.moveTo(b.x + dx, b.y + dy);
        }
        invalidate();
    }
    
    public void paint (Graphics2D g, float scale) {
        for (int i = members.size() - 1; i >= 0; i--)
            ((MapObject)members.get(i)).paint(g, scale);        
        if (isSelected()) {
            g.setStroke(SELECTED_STROKE);
            g.setColor(Color.white);
            g.draw(getBounds());
        }
    }
    
    public void printXml (PrintWriter out) throws IOException {
        out.println("<Group>");
        printMemberXml(out);
        out.println("</Group>");
    }
    
    protected void printMemberXml (PrintWriter out) throws IOException {
        for (Iterator i = members.iterator(); i.hasNext();)
            ((MapObject)i.next()).printXml(out);        
    }
}
