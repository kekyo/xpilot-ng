package org.xpilot.jxpmap;

import java.awt.Dimension;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Properties;

import javax.swing.JOptionPane;

public class MapOptions {


    private Dimension size;
    private Boolean edgeWrap;
    private Map options = new HashMap();
    
    public MapOptions() {
        try {
            Properties props = new Properties();
            props.load(getClass().getResourceAsStream("defopts"));
            for(Iterator i = props.entrySet().iterator(); i.hasNext();) {
                Map.Entry entry = (Map.Entry)i.next();
                Option o = new Option((String)entry.getKey(), 
                    (String)entry.getValue());
                options.put(o.name.toLowerCase(), o);
            }
        } catch (Exception e) {
            JOptionPane.showMessageDialog
                (null, "Failed to load default options: " + 
                 e.getMessage(), "Error",
                 JOptionPane.ERROR_MESSAGE);
            e.printStackTrace();
        }
        getSize();
        isEdgeWrap();                   
    }
    
    public MapOptions(MapOptions mo) {
        this.size = mo.size;
        this.edgeWrap = mo.edgeWrap;
        for(Iterator i = mo.iterator(); i.hasNext();) {
            MapOptions.Option o = (MapOptions.Option)i.next();
            options.put(o.name.toLowerCase(), new Option(o));
        }
        getSize();
        isEdgeWrap();
    }
    
    public Iterator iterator() {
        return options.values().iterator();
    }

    public void set(String name, String value) {
        String key = name.toLowerCase();
        MapOptions.Option o = (MapOptions.Option)options.get(key);
        if (o == null) {
            o = new Option(name, null);
            options.put(key, o);
        }
        o.value = value;
        size = null;
        edgeWrap = null;        
    }
    
    public void reset() {
        for(Iterator i = options.values().iterator(); i.hasNext();) {
            Option o = (Option)i.next();
            o.value = o.defaultValue;
        }  
    }
    
    public void printXml(PrintWriter out) throws IOException {

        out.println("<GeneralOptions>");

        ArrayList list = new ArrayList(options.values());
        Collections.sort(list);
        for (Iterator iter = list.iterator(); iter.hasNext();) {
            Option opt = (Option)iter.next();
            if (!opt.isModified()) continue;
            out.print("<Option name=\"");
            out.print(opt.name);
            out.print("\" value=\"");
            out.print(xmlEncode(opt.value));
            out.println("\"/>");
        }

        out.println("</GeneralOptions>");
    }
    
    public boolean isEdgeWrap() {
        if (edgeWrap == null) { 
            Option o = (Option)options.get("edgewrap");
            if (o == null) {
                edgeWrap = Boolean.FALSE;
                set("edgeWrap", "false");
            } else edgeWrap = new Boolean(o.value);
        }
        return edgeWrap.booleanValue();
    }
    
    public String get(String name) {
        Option o = (Option)options.get(name);
        if (o == null) return null;
        return o.value;
    }
    
    public Dimension getSize() {
        if (size == null) {
            int w = 3500;
            int h = 3500;
            Option wo = (Option)options.get("mapwidth");
            Option ho = (Option)options.get("mapheight");
            if (wo != null) w = Integer.parseInt(wo.value);
            else set("mapWidth", "3500");
            if (ho != null) h = Integer.parseInt(ho.value);
            else set("mapHeight", "3500");
            size = new Dimension(w * 64, h * 64);
        }
        return size;
    }


    private String xmlEncode(String str) {
        StringBuffer sb = new StringBuffer(str.length());
        for (int i = 0; i < str.length(); i++) {
            char ch = str.charAt(i);
            switch (ch) {
            case '&':  sb.append("&amp;");  break;
            case '"':  sb.append("&quot;"); break;
            case '\'': sb.append("&apos;"); break;
            case '<':  sb.append("&lt;");   break;
            case '>':  sb.append("&gt;");   break;
            default:   sb.append(ch);
            }
        }
        return sb.toString();
    }
    
    public class Option implements Comparable {
        public String name;
        public String value;
        public String defaultValue;
        public Option(Option o) {
            this.name = o.name;
            this.value = o.value;
            this.defaultValue = o.defaultValue;
        }
        public Option(String name, String defaultValue) {
            this.name = name;
            this.value = this.defaultValue = defaultValue;
        }
        public boolean isModified() {
            if (value == defaultValue) return false;
            if (value == null && defaultValue != null) return true;
            if (value.equalsIgnoreCase(defaultValue)) return false;
            if ("false".equals(defaultValue))
                return !("no".equalsIgnoreCase(value) 
                    || "off".equalsIgnoreCase(value));
            if ("true".equals(defaultValue))
                return !("yes".equalsIgnoreCase(value)
                    || "on".equalsIgnoreCase(value));
            return true;
        }
        public int compareTo(Object o) {
            return name.compareTo(((Option)o).name);
        }
    }
}
