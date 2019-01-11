/*
 * $Id: SimpleMapObject.java,v 1.1 2003/09/01 07:08:11 juhal Exp $
 */
package org.xpilot.jxpmap;

import java.io.IOException;
import java.io.PrintWriter;

/**
 * @author jli
 */
public class SimpleMapObject extends MapObject {
    
    private String tagName;
    private String imgName;
    
    public SimpleMapObject(String tagName, String imgName) {
        super(imgName, 35 * 64, 35 * 64);
        this.tagName = tagName;
        this.imgName = imgName;
    }

    public void printXml(PrintWriter out) throws IOException {
        out.print('<');
        out.print(tagName);
        out.print(" x=\"");
        out.print(getBounds().x + getBounds().width / 2);
        out.print("\" y=\"");
        out.print(getBounds().y + getBounds().height / 2);
        out.println("\"/>");
    }
    
    public MapObject newInstance() {
        return new SimpleMapObject(tagName, imgName);
    }
    
    public static MapObject createCheck() {
        return new SimpleMapObject("Check", "checkpoint.gif");
    }
    
    public static MapObject createFuel() {
        return new SimpleMapObject("Fuel", "fuel.gif");
    }
    
    public static MapObject createItemConcentrator() {
        return new SimpleMapObject("ItemConcentrator", 
            "itemconcentrator.gif");
    }
    
    public static MapObject createAsteroidConcentrator() {
        return new SimpleMapObject("AsteroidConcentrator",
            "asteroidconcentrator.gif");
    }
}
