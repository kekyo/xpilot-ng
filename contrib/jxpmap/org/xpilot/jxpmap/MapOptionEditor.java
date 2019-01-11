package org.xpilot.jxpmap;

import java.awt.Dimension;
import java.util.*;
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.table.*;
import javax.swing.event.*;

public class MapOptionEditor extends EditorPanel 
implements TableModelListener {

    private DefaultTableModel tableModel;
    private MapOptions options;
    
    public MapOptionEditor(MapOptions options) {
        setTitle("Option Manager");
        this.options = options;
        Vector vec = new Vector();
        for(Iterator i = options.iterator(); i.hasNext();) {
            MapOptions.Option o = (MapOptions.Option)i.next();
            vec.add(o.name);
        }
        Collections.sort(vec);
        JComboBox cb = new JComboBox(vec);
        cb.setEditable(true);
        tableModel = createTableModel(options);
        tableModel.addTableModelListener(this);
        JTable table = new JTable(tableModel);
        table.setRowHeight(24);
        table.getColumnModel().getColumn(0).setCellEditor(
            new DefaultCellEditor(cb));
        Dimension d = table.getPreferredScrollableViewportSize();
        d.height = 200;
        table.setPreferredScrollableViewportSize(d);
        JScrollPane pane = new JScrollPane(table);
        add(pane);
    }




    private DefaultTableModel createTableModel(MapOptions ops) {

        ArrayList rows = new ArrayList();
        for(Iterator i = ops.iterator(); i.hasNext();) {
            MapOptions.Option o = (MapOptions.Option)i.next();
            if (!o.isModified()) continue;
            rows.add(new String[] { o.name, o.value }); 
        }
        Collections.sort(rows, new Comparator () {
                public int compare (Object o1, Object o2) {
                    return ((String[])o1)[0].compareTo(((String[])o2)[0]);
                }
        });
        rows.add(new Object[] { "", "" });
        Object cols[] = new Object[] { "Option", "Value" };
        return new DefaultTableModel(
            (Object[][])rows.toArray(new Object[rows.size()][2]), cols);
    }


    public boolean apply() {
        Vector v = tableModel.getDataVector();
        options.reset();
        for (int i = 0; i < v.size(); i++) {
            Vector row = (Vector)v.get(i);
            String key = row.get(0).toString();
            if (key.length() > 0)
                options.set(key, row.get(1).toString());
        }
        return true;
    }
    
    public void tableChanged(TableModelEvent e) {
        if (e.getColumn() == 0) {
            if (e.getType() == TableModelEvent.UPDATE) {
                int row = e.getLastRow();
                DefaultTableModel tm = (DefaultTableModel)e.getSource();
                if ("".equals(tm.getValueAt(row, 0).toString())) {
                    if (row != tm.getRowCount() - 1) {
                        tm.removeRow(row);
                        return;
                    }
                } else {
                    if (row == tm.getRowCount() - 1) {
                        tm.addRow(new Object[] { "","" }); 
                    }
                }
            }
        }
    }
}
