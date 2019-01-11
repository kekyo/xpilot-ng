package org.xpilot.jxpmap;

import java.awt.Color;
import java.awt.GridLayout;
import java.awt.event.MouseEvent;
import java.awt.event.MouseAdapter;
import java.util.Iterator;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JPanel;
import javax.swing.JColorChooser;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JTextField;

public class PolygonStyleEditor extends EditorPanel {

    private JComboBox cmbFillStyle;
    private JComboBox cmbTexture;
    private JComboBox cmbEdgeStyle;
    private JPanel bColor;
    private JCheckBox cbVisible;
    private JCheckBox cbVisibleInRadar;
    private JTextField tfName;

    private PolygonStyle style;
    private MapModel model;
    private MapCanvas canvas;
    private boolean isNew;

    
    public PolygonStyleEditor (PolygonStyle style, 
                               MapCanvas canvas, 
                               boolean isNew) {
        
        this.style = style;
        this.canvas = canvas;
        this.model = canvas.getModel();
        this.isNew = isNew;
        
        setTitle("Polygon Style");
        setLayout(new GridLayout(7, 2));
        
        add(new JLabel("Name:"));
        tfName = new JTextField();
        if (style.getId() != null) tfName.setText(style.getId());
        add(tfName);
        
        add(new JLabel("Fill:"));
        cmbFillStyle = new JComboBox();
        cmbFillStyle.addItem("None");
        cmbFillStyle.addItem("Color");
        cmbFillStyle.addItem("Texture");
        cmbFillStyle.setSelectedIndex(style.getFillStyle());
        add(cmbFillStyle);
        
        add(new JLabel("Color:"));
        bColor = new JPanel();
        bColor.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
            Color c = JColorChooser.showDialog
                (PolygonStyleEditor.this, "Pick a color", Color.black);
                if (c != null) {
                    bColor.setBackground(c);
                }
            }
        });
        Color c = style.getColor();
        if (c == null) c = Color.black;
        bColor.setBackground(c);
        add(bColor);

        add(new JLabel("Texture:"));
        cmbTexture = new JComboBox();
        cmbTexture.addItem("None");
        int i = 0;
        int j = 1;
        for (Iterator iter = model.pixmaps.iterator(); iter.hasNext(); j++) {
            Pixmap p = (Pixmap)iter.next();
            cmbTexture.addItem(p.getFileName());
            if (style.getTexture() == p) i = j;
        }
        add(cmbTexture);
        cmbTexture.setSelectedIndex(i);
        
        add(new JLabel("Edges:"));
        cmbEdgeStyle = new JComboBox();
        i = 0;
        j = 0;
        for (Iterator iter = model.edgeStyles.iterator(); 
             iter.hasNext(); j++) {
            LineStyle s = (LineStyle)iter.next();
            cmbEdgeStyle.addItem(s.getId());
            if (style.getDefaultEdgeStyle() == s) i = j;
        }
        add(cmbEdgeStyle);
        cmbEdgeStyle.setSelectedIndex(i);
        
        add(new JLabel("Visible:"));
        cbVisible = new JCheckBox();
        cbVisible.setSelected(style.isVisible());
        add(cbVisible);

        add(new JLabel("Radar:"));
        cbVisibleInRadar = new JCheckBox();
        cbVisibleInRadar.setSelected(style.isVisibleInRadar());
        add(cbVisibleInRadar);
    }


    public boolean apply () {

        if (tfName.getText() == null || tfName.getText().length() == 0) {
            JOptionPane.showMessageDialog
                (this, "First specify a name for this style.",
                 "Information", JOptionPane.INFORMATION_MESSAGE);
            return false;
        }
        String id = tfName.getText();
        int fillStyle = cmbFillStyle.getSelectedIndex();
        Color color = bColor.getBackground();
        Pixmap texture = null;
        int i = cmbTexture.getSelectedIndex();
        if (i == 0) {
            if (fillStyle == style.FILL_TEXTURED) {
                JOptionPane.showMessageDialog
                    (this, "Select a texture if you want textured filling.",
                     "Information", JOptionPane.INFORMATION_MESSAGE);
                return false;
            }
        }
        else texture = (Pixmap)model.pixmaps.get(i - 1);
        LineStyle defEdgeStyle = (LineStyle)model.edgeStyles.get
            (cmbEdgeStyle.getSelectedIndex());
        boolean visible = cbVisible.isSelected();
        boolean visibleInRadar = cbVisibleInRadar.isSelected();
        if (isNew) {
            style.setId(id);
            style.setFillStyle(fillStyle);
            style.setColor(color);
            style.setTexture(texture);
            style.setDefaultEdgeStyle(defEdgeStyle);
            model.setDefaultEdgeStyle(defEdgeStyle);
            style.setVisible(visible);
            style.setVisibleInRadar(visibleInRadar);
        } else {
            canvas.setPolygonStyleProperties
                (style,
                 id,
                 fillStyle,
                 color,
                 texture,
                 defEdgeStyle,
                 visible,
                 visibleInRadar);
        }
        return true;
    }
}

 
