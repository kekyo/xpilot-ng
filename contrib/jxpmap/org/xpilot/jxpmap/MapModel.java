package org.xpilot.jxpmap;

import java.awt.Color;
import java.awt.Point;
import java.awt.Polygon;
import java.awt.Rectangle;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringReader;
import java.io.StringWriter;
import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.BufferedInputStream;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.math.BigDecimal;

import javax.xml.parsers.*;
import javax.xml.*;
import org.w3c.dom.*;
import org.xml.sax.*;

import javax.swing.JOptionPane;

public class MapModel extends ModelObject {

    List pixmaps;
    List objects;
    List edgeStyles;
    List polyStyles;
    MapOptions options;
    float defaultScale;
    LineStyle defEdgeStyle;
    Map defPolygonStyles;

    public Object deepClone (Map context) {

        MapModel clone = (MapModel)super.deepClone(context);
            
        ArrayList l = new ArrayList();
        for (Iterator i = pixmaps.iterator(); i.hasNext();)
            l.add(((Pixmap)i.next()).deepClone(context));
        clone.pixmaps = l;

        l = new ArrayList();
        for (Iterator i = objects.iterator(); i.hasNext();)
            l.add(((MapObject)i.next()).deepClone(context));
        clone.objects = l;
        
        l = new ArrayList();
        for (Iterator i = edgeStyles.iterator(); i.hasNext();)
            l.add(((LineStyle)i.next()).deepClone(context));
        clone.edgeStyles = l;
        
        l = new ArrayList();
        for (Iterator i = polyStyles.iterator(); i.hasNext();)
            l.add(((PolygonStyle)i.next()).deepClone(context));
        clone.polyStyles = l;
        
        clone.options = new MapOptions(options);
        return clone;
    }
    
    public MapModel () {

        defaultScale = 1f / 64;
        objects = new ArrayList();
        polyStyles = new ArrayList();
        pixmaps = new ArrayList();
        edgeStyles = new ArrayList();
        options = new MapOptions();

        LineStyle ls;
        ls = new LineStyle("internal", 0, Color.black, LineStyle.STYLE_HIDDEN);
        edgeStyles.add(ls);
        ls = new LineStyle("default", 0, Color.blue, LineStyle.STYLE_SOLID);
        edgeStyles.add(ls);
        defEdgeStyle = ls;
        
        PolygonStyle ps;
        ps = new PolygonStyle();
        ps.setId("default");
        ps.setVisible(true);
        ps.setVisibleInRadar(true);
        ps.setFillStyle(PolygonStyle.FILL_NONE);
        ps.setDefaultEdgeStyle(defEdgeStyle);
        polyStyles.add(ps);
        defPolygonStyles = new HashMap();
        defPolygonStyles.put("default", ps);
    }


    public List getEdgeStyles() {
        return edgeStyles;
    }


    public List getObjects() {
        return objects;
    }


    public MapOptions getOptions() {
        return options;
    }


    public List getPixmaps() {
        return pixmaps;
    }


    public List getPolyStyles() {
        return polyStyles;
    }


    public float getDefaultScale () {
        return defaultScale;
    }


    public void setDefaultScale (float def) {
        this.defaultScale = def;
    }


    public LineStyle getDefaultEdgeStyle () {
        return defEdgeStyle;
    }


    public void setDefaultEdgeStyle (LineStyle style) {
        this.defEdgeStyle = style;

    }


    public Map getDefaultPolygonStyles() {
        return defPolygonStyles;
    }
    
    
    public void setDefaultPolygonStyles (Map styles) {
        this.defPolygonStyles = styles;
    }


    public void addToFront (MapObject moNew) {
        addObject(moNew, true);
    }


    public void addToBack (MapObject moNew) {
        addObject(moNew, false);
    }


    public void addObject (MapObject moNew, boolean front) {

        for (ListIterator iter = objects.listIterator(); iter.hasNext();) {
            MapObject moOld = (MapObject)iter.next();
            int znew = moNew.getZOrder();
            int zold = moOld.getZOrder();
            
            if (znew < zold || (front && znew == zold)) {
                iter.previous();
                iter.add(moNew);
                return;
            }
        }
        objects.add(moNew);
    }

    public void addObject (int index, MapObject mo) {
        objects.add(index, mo);
    }
    
    public void replace (MapObject from, MapObject to) {
        int i = objects.indexOf(from);
        objects.set(i, to);
    }
    
    public int indexOf (MapObject mo) {
        return objects.indexOf(mo);
    }

    public void removeObject (MapObject mo) {
        objects.remove(mo);
    }

    public void removeObject (int index) {
        objects.remove(index);
    }
    
    public Object[] validateModel () {
        for (Iterator i = objects.iterator(); i.hasNext();) {
            MapObject o = (MapObject)i.next();
            if (o instanceof MapPolygon) {
                Polygon p = ((MapPolygon)o).getPolygon();
                if (p.npoints < 1) continue;
                int x = p.xpoints[0];
                int y = p.ypoints[0];
                if (x < 0 || x >= options.getSize().width
                || y < 0 || y >= options.getSize().height) {
                    return new Object[] {
                    o, "Polygon is located outside map bounds." };
                }                
            } else if (!(o instanceof Group)) {
                Rectangle b = o.getBounds();
                if (b.x < 0 || b.x >= options.getSize().width
                || b.y < 0 || b.y >= options.getSize().height) {
                    return new Object[] {
                    o, "Object is located outside map bounds." };
                }
            }
        }
        return null;
    }


    public void load (String name) 
    throws IOException, ParserConfigurationException,
    SAXException, ParseException, XPDFile.ParseException {
            
        edgeStyles.clear();
        LineStyle ls;
        ls = new LineStyle("internal", 0, Color.black, LineStyle.STYLE_HIDDEN);
        edgeStyles.add(ls);
        polyStyles.clear();
        pixmaps.clear();

        XPDFile xpd = XPDFile.load(new File(name));
        XPDFile.Part first = (XPDFile.Part)xpd.get(0);
        readXml(new String(first.data, "ISO8859_1"));

        for (Iterator i = pixmaps.iterator(); i.hasNext();) {
            Pixmap pixmap = (Pixmap)i.next();
            for (Iterator j = xpd.iterator(); j.hasNext();) {
                XPDFile.Part part = (XPDFile.Part)j.next();
                if (pixmap.getFileName().equals(part.name))
                    pixmap.load(new ByteArrayInputStream(part.data));
            }
            if (pixmap.getImage() == null)
                throw new IOException("missing image: " 
                    + pixmap.getFileName());
        }

        defEdgeStyle = (LineStyle)edgeStyles.get(edgeStyles.size() - 1);
        defPolygonStyles = new HashMap();
        defPolygonStyles.put("default", polyStyles.get(polyStyles.size() - 1));
    }
    
    /*
    private void readXml (String xml) 
    throws IOException, SAXException, ParserConfigurationException {
        SAXParserFactory spf = SAXParserFactory.newInstance();
        spf.setValidating(false);
        XMLReader reader = spf.newSAXParser().getXMLReader();
        reader.setContentHandler(new MapDocumentHandler());
        reader.parse(new InputSource(new StringReader(xml)));
    }
    */

    public void save (File file) throws IOException {
        
        String xml = exportXml();
        
        String xmlName = file.getName();
        int dot = xmlName.lastIndexOf('.');
        if (dot != -1) xmlName = xmlName.substring(0, dot);
        xmlName += ".xp2";
        
        XPDFile xpd = new XPDFile();
        xpd.add(new XPDFile.Part(xmlName, xml.getBytes("ISO8859_1")));
        for (Iterator i = pixmaps.iterator(); i.hasNext();) {
            Pixmap pixmap = (Pixmap)i.next();
            xpd.add(new XPDFile.Part(pixmap.getFileName(), 
                new PPMEncoder().encode(pixmap.getImage())));
        }
        
        xpd.save(file);
    }
    
    
    private String downloadImages() throws IOException {
        String urlStr = options.get("dataurl");
        if (urlStr == null) {
            JOptionPane.showMessageDialog(null,
                "The map has no dataURL option");
            return "";
        }
        File tmpDir = File.createTempFile("jxpmap", null);
        // JDK seems to create an actual file
        // I need to delete it before it can be used as a dir
        tmpDir.delete();
        if (!tmpDir.mkdir())
            throw new IOException("unable to create temporary directory: " 
                + tmpDir.getAbsolutePath());
        tmpDir.deleteOnExit();
        URL url = new URL(urlStr);
        File xpdFile = new File(tmpDir, new File(url.getPath()).getName());        
        InputStream in = url.openStream();
        try {
            FileOutputStream out = new FileOutputStream(xpdFile);
            try {
                byte buf[] = new byte[2048];
                for (int c = in.read(buf); c != -1; c = in.read(buf))
                    out.write(buf, 0, c);
            } finally {
                out.close();
            }
        } finally {
            in.close();
        }
        try {
            XPDFile xpd = XPDFile.load(xpdFile);
            xpdFile.delete();
            for (Iterator i = xpd.iterator(); i.hasNext();) {
                XPDFile.Part p = (XPDFile.Part)i.next();
                File pf = new File(tmpDir, p.name);
                FileOutputStream out = new FileOutputStream(pf);
                try {
                    out.write(p.data);
                } finally {
                    out.close();
                }
                pf.deleteOnExit();
            }
            return tmpDir.getAbsolutePath() + "/";
        } catch (XPDFile.ParseException pe) {
            throw new IOException("corrupted xpd file");
        }
    }
    
    
    public void importXml(String xml)
    throws IOException, ParseException, 
    SAXException, ParserConfigurationException {
        edgeStyles.clear();
        LineStyle ls;
        ls = new LineStyle("internal", 0, Color.black, LineStyle.STYLE_HIDDEN);
        edgeStyles.add(ls);
        polyStyles.clear();
        pixmaps.clear();

        readXml(xml);

        boolean asked = false;
        String tmpDir = "";
        for (Iterator i = pixmaps.iterator(); i.hasNext();) {
            Pixmap pixmap = (Pixmap)i.next();
            File f = new File(tmpDir + pixmap.getFileName());
            if (!asked && !f.exists()) {
                asked = true;
                if (JOptionPane.showConfirmDialog(null,
                    "Download the images used by this map?") 
                    == JOptionPane.YES_OPTION) {
                    tmpDir = downloadImages();
                    f = new File(tmpDir + pixmap.getFileName());
                }
            }
            if (f.exists()) {
                InputStream in = new BufferedInputStream(
                    new FileInputStream(f));
                try {
                    pixmap.load(in);
                } finally {
                    in.close();
                }
            }
        }

        defEdgeStyle = (LineStyle)edgeStyles.get(edgeStyles.size() - 1);
        defPolygonStyles.put("default", polyStyles.get(polyStyles.size() - 1));
    }
    
    public String exportXml() throws IOException {
        
        StringWriter w = new StringWriter();
        PrintWriter out = new PrintWriter(w);
        
        out.println("<XPilotMap version=\"1.1\">");  
        options.printXml(out);
        for (Iterator iter = pixmaps.iterator(); iter.hasNext();) {
            Pixmap p = (Pixmap)iter.next();
            p.printXml(out);
        }
        for (Iterator iter = edgeStyles.iterator(); iter.hasNext();) {
            LineStyle s = (LineStyle)iter.next();
            s.printXml(out);
        }
        for (Iterator iter = polyStyles.iterator(); iter.hasNext();) {
            PolygonStyle s = (PolygonStyle)iter.next();
            s.printXml(out);
        }
        for (int i = objects.size() - 1; i >= 0; i--) {
            ((MapObject)objects.get(i)).printXml(out);
        }
        out.println("</XPilotMap>");

        return w.toString();        
    }
    
    private String atts(Node tag, String name) {
        Node att = tag.getAttributes().getNamedItem(name);
        return att != null ? att.getNodeValue() : null;
    }
    
    private int atti(Node tag, String name) {
        return Integer.parseInt(atts(tag, name));
    }
    
    private int atti(Node tag, String name, int def) {
        String s = atts(tag, name);
        if (s == null) return def;
        return Integer.parseInt(s);
    }
    
    private Iterator tags(Node parent, String name) {
        ArrayList list = new ArrayList();
        for (Node n = parent.getFirstChild(); 
        n != null; 
        n = n.getNextSibling()) {
            if (n.getNodeType() != Node.ELEMENT_NODE) continue;
            if (!name.equalsIgnoreCase(n.getNodeName())) continue;
            list.add(n);
        }
        return list.iterator();
    }
    
    private void readXml(String xml)
    throws IOException, SAXException, 
    ParseException, ParserConfigurationException {
        Document doc = 
            DocumentBuilderFactory.newInstance().newDocumentBuilder().parse(
                new InputSource(new StringReader(xml)));
        Element root = doc.getDocumentElement();
        if (!"xpilotmap".equalsIgnoreCase(root.getNodeName()))
            throw new ParseException("expected XPilotMap");
        if (root.getFirstChild() == null) return;
        
        Map bstyles = new HashMap();
        for (Iterator i = tags(root, "bmpstyle"); i.hasNext();) {
            Node n = (Node)i.next();
            Pixmap pm = new Pixmap();
            pm.setFileName(atts(n, "filename"));
            String s = atts(n, "scalable");
            if (s != null && !"yes".equals(s))
                pm.setScalable(false);
            bstyles.put(atts(n, "id"), pm);
            pixmaps.add(pm);
        }
        
        Map estyles = new HashMap();
        for (Iterator i = tags(root, "edgestyle"); i.hasNext();) {
            Node n = (Node)i.next();
            LineStyle ls = new LineStyle();
            ls.setId(atts(n, "id"));
            ls.setWidth(atti(n, "width"));
            ls.setStyle(atti(n, "style"));
            if (ls.getWidth() == -1) {
                ls.setWidth(1);
                ls.setStyle(LineStyle.STYLE_HIDDEN);
            }
            ls.setColor(new Color(Integer.parseInt(atts(n, "color"), 16)));
            estyles.put(ls.getId(), ls);
            edgeStyles.add(ls);
        }
        
        Map pstyles = new HashMap();
        for (Iterator i = tags(root, "polystyle"); i.hasNext();) {
            Node n = (Node)i.next();
            PolygonStyle style = new PolygonStyle();                             
            style.setId(atts(n, "id"));
            String cstr = atts(n, "color");
            if (cstr != null)
                style.setColor(
                    new Color(Integer.parseInt(cstr, 16)));      
            String txid = atts(n, "texture");
            if (txid != null)
                style.setTexture((Pixmap)bstyles.get(txid));
            style.parseFlags(atti(n, "flags", 1));
            LineStyle ls = null;
            String lsid = atts(n, "defedge");
            if (lsid != null) ls = (LineStyle)estyles.get(lsid);
            if (ls == null)
                throw new ParseException(
                    "Undefined edge style: " + lsid);
            style.setDefaultEdgeStyle(ls);
            polyStyles.add(style);
            pstyles.put(style.getId(), style);            
        }
        
        Group temp = new Group();
        readObjects(temp, root, pstyles, estyles);
        objects.addAll(temp.getMembers());
    }
    
    private void 
    readObjects(Group parent, Node root, Map pstyles, Map estyles)
    throws ParseException {
        
        for (Node n = root.getFirstChild(); n != null; 
        n = n.getNextSibling()) {
            
            if (n.getNodeType() != Node.ELEMENT_NODE) continue;
            String name = n.getNodeName();
            
            if (name.equalsIgnoreCase("generaloptions")) {
                
                for (Node n2 = n.getFirstChild(); n2 != null; 
                n2 = n2.getNextSibling()) {
                    if (n2.getNodeType() != Node.ELEMENT_NODE) continue;
                    if (!"option".equalsIgnoreCase(n2.getNodeName())) {
                        System.err.println("unknown tag inside options: "
                            + n2.getNodeName());
                        continue;
                    }
                    options.set(atts(n2, "name"), atts(n2, "value"));
                }
                
            } else if (name.equalsIgnoreCase("polygon")) {
                
                PolygonStyle s = (PolygonStyle)pstyles.get(atts(n, "style"));
                if (s == null) throw new ParseException(
                    "undefined style: " + atts(n, "style")); 
                ArrayList points = new ArrayList();
                int x = atti(n, "x");
                int y = atti(n, "y");
                points.add(new Point(x, y));
                ArrayList edges = new ArrayList();
                HashMap states = new HashMap();
                states.put("default", s);
                boolean special = false;
                for (Node n2 = n.getFirstChild(); n2 != null; 
                n2 = n2.getNextSibling()) {
                    if (n2.getNodeType() != Node.ELEMENT_NODE) continue;
                    String name2 = n2.getNodeName();
                    if (name2.equalsIgnoreCase("offset")) {
                        x += atti(n2, "x");
                        y += atti(n2, "y");
                        points.add(new Point(x, y));
                        LineStyle es = null;
                        String esName = atts(n2, "style");
                        if (esName != null) 
                            es = (LineStyle)estyles.get(esName);
                        if (es != null) special = true;
                        edges.add(es);
                    } else if (name2.equalsIgnoreCase("style")) {
                        PolygonStyle ps = 
							(PolygonStyle)pstyles.get(atts(n2, "id"));
                        if (ps != null) states.put(atts(n2, "state"), ps);
                        else System.err.println(
                            "undefined state style: " + atts(n2, "id"));
                    } else {
                        System.err.println("unknown tag inside polygon: "
                        + name2);
                    }
                }
                Polygon p = new Polygon();
                for (int i = 0; i < points.size() - 1; i++) {
                    Point pnt = (Point)points.get(i);
                    p.addPoint(pnt.x, pnt.y);
                }
                MapPolygon mp = 
                    new MapPolygon(p, states, special ? edges : null);
                parent.addToFront(mp);
                
            } else if (name.equalsIgnoreCase("fuel")) {

                MapObject o = SimpleMapObject.createFuel();
                Rectangle r = o.getBounds();
                o.moveTo(atti(n, "x") - r.width / 2, 
                    atti(n, "y") - r.height / 2);
                parent.addToFront(o);

            } else if (name.equalsIgnoreCase("ball")) {

                PolygonStyle style = null;
                String styleName = atts(n, "style");
                if (styleName != null) {
                    style = (PolygonStyle)pstyles.get(styleName);
                    if (style == null)
                        System.err.println("undefined style: " 
                            + styleName);
                }
                Ball o = new Ball(
                    atti(n, "x"),
                    atti(n, "y"),
                    atti(n, "team", 0xffff),
                    style);
                Rectangle r = o.getBounds();
                o.moveTo(r.x - r.width / 2, r.y - r.height / 2);
                parent.addToFront(o);

            } else if (name.equalsIgnoreCase("base")) {

                Base o = new Base(
                    atti(n, "x"),
                    atti(n, "y"),
                    atti(n, "dir"),
                    atti(n, "team", -1),
                    atti(n, "order", 0));
                Rectangle r = o.getBounds();
                o.moveTo(r.x - r.width / 2, r.y - r.height / 2);
                parent.addToFront(o);

            } else if (name.equalsIgnoreCase("check")) {

                MapObject o = SimpleMapObject.createCheck();
                Rectangle r = o.getBounds();
                o.moveTo(atti(n, "x") - r.width / 2, 
                    atti(n, "y") - r.height / 2);
                parent.addToFront(o);
                
            } else if (name.equalsIgnoreCase("itemconcentrator")) {

                MapObject o = SimpleMapObject.createItemConcentrator();
                Rectangle r = o.getBounds();
                o.moveTo(atti(n, "x") - r.width / 2, 
                    atti(n, "y") - r.height / 2);
                parent.addToFront(o);

            } else if (name.equalsIgnoreCase("asteroidconcentrator")) {

                MapObject o = SimpleMapObject.createAsteroidConcentrator();
                Rectangle r = o.getBounds();
                o.moveTo(atti(n, "x") - r.width / 2, 
                    atti(n, "y") - r.height / 2);
                parent.addToFront(o);
                
            } else if (name.equalsIgnoreCase("grav")) {

                Grav g = new Grav(
                    atti(n, "x"),
                    atti(n, "y"),
                    atts(n, "type"),
                    atts(n, "force"));
                Rectangle r = g.getBounds();
                g.moveTo(r.x - r.width / 2, r.y - r.height / 2);
                parent.addToFront(g);

            } else if (name.equalsIgnoreCase("wormhole")) {

                Wormhole wh = new Wormhole();
                wh.setPoint(atti(n, "x"), atti(n, "y"));
                wh.setType(atts(n, "type"));
                readObjects(wh, n, pstyles, estyles);
                parent.addToFront(wh);

            } else if (name.equalsIgnoreCase("ballarea")) {
                
                BallArea ba = new BallArea();
                readObjects(ba, n, pstyles, estyles);
                parent.addToFront(ba);

            } else if (name.equalsIgnoreCase("balltarget")) {

                BallTarget bt = new BallTarget();
                bt.setTeam(atti(n, "team"));
                readObjects(bt, n, pstyles, estyles);
                parent.addToFront(bt);

            } else if (name.equalsIgnoreCase("decor")) {
                
                Decoration d = new Decoration();
                readObjects(d, n, pstyles, estyles);
                parent.addToFront(d);

            } else if (name.equalsIgnoreCase("cannon")) {
                
                Cannon c = new Cannon();
                c.setTeam(atti(n, "team", -1));
                c.setPoint(atti(n, "x"), atti(n, "y"));
                c.setDir(atti(n, "dir"));
                readObjects(c, n, pstyles, estyles);
                parent.addToFront(c);                
                
            } else if (name.equalsIgnoreCase("target")) {
                
                Target t = new Target();
                t.setTeam(atti(n, "team", -1));
                readObjects(t, n, pstyles, estyles);
                parent.addToFront(t);
                
            } else if (name.equalsIgnoreCase("frictionarea")) {
                
                FrictionArea fa = new FrictionArea();
                fa.setFriction(new BigDecimal(atts(n, "friction")));
                readObjects(fa, n, pstyles, estyles);
                parent.addToFront(fa);
                
            } else if (name.equalsIgnoreCase("group")) {
                
                Group g = new Group();
                readObjects(g, n, pstyles, estyles);
                parent.addToFront(g);
                
            } else if (name.equalsIgnoreCase("bmpstyle") 
            || name.equalsIgnoreCase("edgestyle") 
            || name.equalsIgnoreCase("polystyle")) {
                /* ignore */
            } else {
                System.err.println("unknown map element: " + name);
            }
        }
    }
    
    public static class ParseException extends Exception {
        public ParseException(String msg) {
            super(msg);
        }
    }    
}
