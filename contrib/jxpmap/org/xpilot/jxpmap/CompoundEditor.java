/*
 * $Id: CompoundEditor.java,v 1.1 2003/08/24 18:37:36 juhal Exp $
 */
package org.xpilot.jxpmap;

import java.awt.BorderLayout;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import javax.swing.JTabbedPane;

/**
 * @author jli
 */
public class CompoundEditor extends EditorPanel {
    private MapCanvas canvas;
    private MapObject object;
    private JTabbedPane tabbedPane;
    private List editors;

    public CompoundEditor(String title, MapCanvas canvas, MapObject object) {
        setTitle(title);
        this.canvas = canvas;
        this.object = object;
        setLayout(new BorderLayout());
        tabbedPane = new JTabbedPane();
        add(tabbedPane, BorderLayout.CENTER);
        editors = new ArrayList();
    }

    public void add(EditorPanel editor) {
        tabbedPane.addTab(editor.getTitle(), editor);
        editors.add(editor);
    }

    public boolean apply () {
        canvas.beginEdit();
        boolean ok = true;
        for (Iterator i = editors.iterator(); i.hasNext();) {
            if (!((EditorPanel)i.next()).apply()) {
                ok = false;
                break;
            }
        }
        if (ok) canvas.commitEdit();
        else canvas.cancelEdit();
        return ok;
    }

    public boolean cancel () {
        boolean ok = true;
        for (Iterator i = editors.iterator(); i.hasNext();) {
            if (!((EditorPanel)i.next()).apply()) {
                ok = false;
                break;
            }
        }
        return ok;
    }
}
