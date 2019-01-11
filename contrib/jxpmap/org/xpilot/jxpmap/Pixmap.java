package org.xpilot.jxpmap;

import java.awt.image.BufferedImage;
import java.io.InputStream;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.PrintWriter;

public class Pixmap extends ModelObject {
    
    private String fileName;
    private boolean scalable;
    private BufferedImage image;

    public Pixmap () {
        this.scalable = true;
    }

    
    public String getFileName () {
        return fileName;
    }
    
    public void setFileName (String  f) {
        this.fileName = f;
    }
        
    public boolean isScalable () {
        return scalable;
    }
    
    public void setScalable (boolean s) {
        this.scalable = s;
    }
    
    
    public BufferedImage getImage () {
        return image;
    }
    
    
    public void setImage (BufferedImage image) {
        this.image = image;
    }
    
    
    public void load (InputStream in) throws IOException {
        image = 
            new PPMDecoder().decode
                (new BufferedInputStream(in));
    }
    

    public void printXml (PrintWriter out) throws IOException {
        out.print("<BmpStyle id=\"");
        out.print(getFileName());
        out.print("\" filename=\"");
        out.print(getFileName());
        out.println(isScalable() 
            ? "\" scalable=\"yes\"/>" 
            : "\" scalable=\"no\"/>");
    }
}
