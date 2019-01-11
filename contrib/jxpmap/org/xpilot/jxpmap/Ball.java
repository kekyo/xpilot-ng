package org.xpilot.jxpmap;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Iterator;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;

public class Ball extends MapObject {
    
    private PolygonStyle style;
    private Image original;

    public Ball () {
        this(0, 0, 1, null);
    }

    
    public Ball (int x, int y, int team, PolygonStyle style) {
        super("ball.gif", x, y, 21 * 64, 21 * 64);
        setTeam(team);
        setStyle(style);
    }
    
    
    public PolygonStyle getStyle () {
        return style;
    }
    
    
    public void setStyle (PolygonStyle style) {
        this.style = style;
        Color c = Color.green;
        if (style != null && style.getColor() != null)
            c = style.getColor();
        if (this.original == null) this.original = this.img;
        this.img = Toolkit.getDefaultToolkit().createImage(
            new FilteredImageSource(this.original.getSource(),
                new BlendImageFilter(c)));
	new javax.swing.ImageIcon(this.img); /* force image production */
    }

    
    public void printXml (PrintWriter out) throws IOException {
        out.print("<Ball x=\"");
        out.print(getBounds().x + getBounds().width / 2);
        out.print("\" y=\"");
        out.print(getBounds().y + getBounds().height / 2);
        if (getStyle() != null) {
            out.print("\" style=\"");
            out.print(getStyle().getId());
        }
        out.print("\" team=\"");
        out.print(getTeam());
        out.println("\"/>");
    }
    
    
    public EditorPanel getPropertyEditor(MapCanvas canvas) {
        return new BallPropertyEditor(canvas);
    }    


    private class BallPropertyEditor extends EditorPanel {

        private JComboBox cmbTeam;
        private JComboBox cmbStyle;
        private MapCanvas canvas;


        public BallPropertyEditor (MapCanvas canvas) {

            setTitle("Ball");

            cmbTeam = new JComboBox();
            for (int i = -1; i <= 10; i++) 
                cmbTeam.addItem(new Integer(i));
            cmbTeam.setSelectedIndex(getTeam() + 1);
            
            cmbStyle = new JComboBox();
            cmbStyle.addItem("None");
            for (Iterator iter = canvas.getModel().polyStyles.iterator(); 
            iter.hasNext();) {
                PolygonStyle style = (PolygonStyle)iter.next();
                cmbStyle.addItem(style.getId());
            }
            if (Ball.this.getStyle() != null)
                cmbStyle.setSelectedItem(Ball.this.getStyle().getId());
            else
                cmbStyle.setSelectedIndex(0);
            
            setLayout(new GridLayout(2,2));
            add(new JLabel("Team:"));
            add(cmbTeam);
            add(new JLabel("Style:"));
            add(cmbStyle);

            this.canvas = canvas;
        }
        
        
        public boolean apply () {
            int newTeam = cmbTeam.getSelectedIndex() - 1;
            int styleIndex = cmbStyle.getSelectedIndex();
            PolygonStyle newStyle = styleIndex == 0 ? null :
                (PolygonStyle)canvas.getModel().polyStyles.get(styleIndex - 1);
            if (newTeam != getTeam() || newStyle != getStyle())
                canvas.setBallProperties(Ball.this, newTeam, newStyle);
            return true;
        }
    }
    
    private class BlendImageFilter extends RGBImageFilter {
        
        private int r, g, b;
        
        public BlendImageFilter(Color c) {
            int rgb = c.getRGB();
            r = (rgb >> 16) & 0xff;
            g = (rgb >> 8) & 0xff;
            b = rgb & 0xff;
            canFilterIndexColorModel = true;
        }
        
	    public int filterRGB(int x, int y, int rgb) {
            return (((rgb >> 16) & 0xff) * r / 0xff) << 16
            | (((rgb >> 8) & 0xff) * g / 0xff) << 8
            | ((rgb & 0xff) * b / 0xff)
            | 0xff000000;
	    }
    }
}
