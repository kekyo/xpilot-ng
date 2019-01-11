package org.xpilot.jxpmap;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Polygon;
import java.awt.Rectangle;
import java.awt.TexturePaint;
import java.awt.image.BufferedImage;

import javax.swing.JComponent;

public class PolygonStylePreview extends JComponent {

    private PolygonStyle style;


    public void setStyle (PolygonStyle style) {
        this.style = style;
        repaint();
    }


    public void paint (Graphics _g) {
        Graphics2D g = (Graphics2D)_g;
        g.setColor(Color.black);
        g.fill(g.getClipBounds());

        if (style == null) return;
        if (!style.isVisible()) return;

        Polygon p = getPreviewPolygon();
        
        if (style.getFillStyle() == PolygonStyle.FILL_COLOR) {
            g.setColor(style.getColor());
            g.fillPolygon(p);
            
        } else if (style.getFillStyle() == PolygonStyle.FILL_TEXTURED) {
            BufferedImage img = style.getTexture().getImage();
            Rectangle b = p.getBounds();
            g.setPaint(new TexturePaint(img, new Rectangle
                (b.x, b.y, img.getWidth(), img.getHeight())));
            g.fill(p);
        }

        LineStyle def = style.getDefaultEdgeStyle();
        if (def.getStyle() != LineStyle.STYLE_HIDDEN) {
            g.setColor(def.getColor());
            g.setStroke(def.getStroke(1.0f));
            g.draw(p);
        }
    }

    
    private Polygon getPreviewPolygon () {
        
        Polygon p = new Polygon();
        Dimension d = getSize();
        int a = Math.min(d.width, d.height);
        float u = a / 6f;

        int x = (d.width - a) / 2;
        int y = (d.height - a) / 2;
        
        for (int i = 0; i < XPOINTS.length; i++) {
            p.addPoint(x + (int)(XPOINTS[i] * u), y + (int)(YPOINTS[i] * u));
        }

        return p;
    }


    private static final int[] XPOINTS = {1, 1, 5, 5, 3, 4, 2, 3, 1};
    private static final int[] YPOINTS = {1, 5, 5, 3, 4, 2, 3, 1, 1};
}
