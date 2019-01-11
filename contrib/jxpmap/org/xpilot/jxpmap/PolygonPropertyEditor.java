package org.xpilot.jxpmap;

import java.awt.*;
import java.util.*;
import javax.swing.*;

public class PolygonPropertyEditor extends EditorPanel {

    private MapPolygon polygon;
    private MapModel model;
    private MapCanvas canvas;
    private JComboBox cmbDefStyle, cmbDestStyle;
    
    public PolygonPropertyEditor (MapCanvas canvas, MapPolygon poly) {

        this.polygon = poly;
        this.canvas = canvas;
        this.model = canvas.getModel();

        setTitle("Polygon");
        setLayout(new GridLayout(2,2));
        
        cmbDefStyle = new JComboBox();
        for (Iterator iter = model.polyStyles.iterator(); iter.hasNext();)
            cmbDefStyle.addItem(((PolygonStyle)iter.next()).getId());
        cmbDefStyle.setSelectedItem(poly.getDefaultStyle().getId());

        cmbDestStyle = new JComboBox();
        cmbDestStyle.addItem("");
        for (Iterator iter = model.polyStyles.iterator(); iter.hasNext();)
            cmbDestStyle.addItem(((PolygonStyle)iter.next()).getId());
        if (poly.getStyle("destroyed") != null)
            cmbDestStyle.setSelectedItem(poly.getStyle("destroyed").getId());
        
        add(new JLabel("Default style:"));
        add(cmbDefStyle);
        add(new JLabel("Destroyed style:"));
        add(cmbDestStyle);
    }


    public boolean apply () {
        Map m = new HashMap();
        m.put("default", model.polyStyles.get(cmbDefStyle.getSelectedIndex()));
        if (cmbDestStyle.getSelectedIndex() > 0)
            m.put("destroyed", 
                model.polyStyles.get(cmbDestStyle.getSelectedIndex() - 1));
        polygon.setCurrentStyle(null);
        canvas.setPolygonStyles(polygon, m);
        return true;
    }
}
