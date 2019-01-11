package org.xpilot.jxpmap;

import java.awt.Color;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Map;

public class PolygonStyle extends ModelObject {

    public static final int FILL_NONE = 0;
    public static final int FILL_COLOR = 1;
    public static final int FILL_TEXTURED = 2;

    private boolean visible = true;
    private boolean visibleInRadar = true;
    private Color color;
    private Pixmap texture;
    private int fillStyle;
    private LineStyle defEdgeStyle;
    private String id;

    public Object deepClone (Map context) {
        PolygonStyle ps = (PolygonStyle)super.deepClone(context);
        if (texture != null)
            ps.texture = (Pixmap)texture.deepClone(context);
        if (defEdgeStyle != null)
            ps.defEdgeStyle = (LineStyle)defEdgeStyle.deepClone(context);
        return ps;
    }


    public boolean isVisible () {
        return visible;
    }


    public void setVisible (boolean v) {
        this.visible = v;
    }


    public boolean isVisibleInRadar () {
        return visibleInRadar;
    }


    public void setVisibleInRadar (boolean v) {
        this.visibleInRadar = v;
    }

    
    public Color getColor () {
        return color;
    }


    public void setColor (Color c) {
        this.color = c;
    }


    public Pixmap getTexture () {
        return texture;
    }


    public void setTexture (Pixmap t) {
        this.texture = t;
    }


    public int getFillStyle () {
        return fillStyle;
    }


    public void setFillStyle (int f) {
        this.fillStyle = f;
    }


    public LineStyle getDefaultEdgeStyle () {
        return defEdgeStyle;
    }


    public void setDefaultEdgeStyle (LineStyle l) {
        this.defEdgeStyle = l;
    }


    public String getId () {
        return id;
    }

    
    public void setId (String id) {
        this.id = id;
    }


    public void printXml (PrintWriter out) throws IOException {

        out.print("<PolyStyle id=\"");
        out.print(id);

        if (color != null) {
            out.print("\" color=\"");
            out.print(toRgb(color));
        }
        if (fillStyle == FILL_TEXTURED && texture != null) {
            out.print("\" texture=\"");
            out.print(texture.getFileName());
        }
        out.print("\" defedge=\"");
        out.print(defEdgeStyle.getId());
        out.print("\" flags=\"");
        out.print(computeFlags());
        out.println("\"/>");
    }

    
    private int computeFlags () {
        int flags = 0;
        if (fillStyle != FILL_NONE) {
            if (color != null) flags |= 1;
            if (texture != null) flags |= 2;
        }
        if (!isVisible()) flags |= 4;
        if (!isVisibleInRadar()) flags |= 8;
        return flags;
    }
    
    
    public void parseFlags (int flags) {
        fillStyle = FILL_NONE;
        if ((flags & 1) != 0) fillStyle = FILL_COLOR;
        if ((flags & 2) != 0) fillStyle = FILL_TEXTURED;
        if ((flags & 4) != 0) setVisible(false);
        if ((flags & 8) != 0) setVisibleInRadar(false);
    }

    
    public static String toRgb (Color c) {
        String s = Integer.toHexString(c.getRGB() & 0xffffff).toUpperCase();
        if (s.length() == 6) return s;
        StringBuffer sb = new StringBuffer(6);
        for (int i = 0; i < 6 - s.length(); i++) sb.append('0');
        return sb.append(s).toString();
    }
}
