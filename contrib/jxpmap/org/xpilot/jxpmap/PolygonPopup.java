/*
 * $Id: PolygonPopup.java,v 1.1 2003/08/24 18:37:36 juhal Exp $
 */
package org.xpilot.jxpmap;

import java.awt.event.ActionEvent;

import javax.swing.JMenuItem;

/**
 * @author jli
 */
public class PolygonPopup extends MapObjectPopup {
    
    protected MapPolygon poly;
    private JMenuItem itemRotate;

    public PolygonPopup(MapPolygon poly) {
        super(poly);
        this.poly = poly;
        JMenuItem item = new JMenuItem("Rotate");
        item.addActionListener(this);
        add(item);
        itemRotate = item;
    }
    
    public void actionPerformed(ActionEvent ae) {
        if (ae.getSource() == itemRotate) {
            canvas.setCanvasEventHandler(poly.getRotateHandler());
        } else {
            super.actionPerformed(ae);
        }
    }
}
