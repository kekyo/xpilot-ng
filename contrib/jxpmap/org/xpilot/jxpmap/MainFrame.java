package org.xpilot.jxpmap;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import javax.swing.AbstractAction;
import javax.swing.ActionMap;
import javax.swing.ButtonGroup;
import javax.swing.ImageIcon;
import javax.swing.InputMap;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JToggleButton;
import javax.swing.JToolBar;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.KeyStroke;
import javax.swing.SwingConstants;

public class MainFrame extends JFrame implements ActionListener {
    
    private static final Dimension BUTTON_SIZE = new Dimension(26, 26);
    
    private MapCanvas canvas;
    private int zoom;
    
    private ButtonGroup toggleGroup;
    private JLabel lblZoom;
    private JSpinner gridSpinner;
    private JToggleButton gridToggle;
    private File mapFile;
    private BshConsole bshConsole;
    private JMenu scriptMenu;
    private bsh.Interpreter interpreter;
    
    public MainFrame() throws Exception {
        super("XPilotNG Map Editor");
        canvas = new MapCanvas();
        getContentPane().add(canvas, BorderLayout.CENTER);
        buildMenuBar();
        buildToolBar();
        buildActionMap();
        buildInputMap();
        setSize(800, 600);
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        Dimension ss = Toolkit.getDefaultToolkit().getScreenSize();
        setLocation(ss.width / 2 - 400, ss.height / 2 - 300);
        interpreter = new bsh.Interpreter();
        interpreter.set("app", this);
        interpreter.set("editor", canvas);
        interpreter.set("scriptMenu", scriptMenu);
        InputStream in = getClass().getResourceAsStream("init.bsh");
        if (in != null) {
            try {
                interpreter.eval(new InputStreamReader(in));
            } finally {
                in.close();
            }
        }        
    }
    
    
    public void setModel (MapModel model) {
        canvas.setModel(model);
    }
    
    
    private void buildMenuBar() {
        
        JMenu menu;
        JMenuItem menuItem;
        JMenuBar menuBar = new JMenuBar();
        setJMenuBar(menuBar);
        
        menu = new JMenu("File");
        menuBar.add(menu);
        
        menuItem = new JMenuItem("New");
        menu.add(menuItem);
        menuItem.setActionCommand("newMap");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("Open");
        menu.add(menuItem);
        menuItem.setActionCommand("openMap");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("Save");
        menu.add(menuItem);
        menuItem.setActionCommand("saveMap");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("Import XML");
        menu.add(menuItem);
        menuItem.setActionCommand("importXml");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("Export XML");
        menu.add(menuItem);
        menuItem.setActionCommand("exportXml");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("Exit");
        menu.add(menuItem);
        menuItem.setActionCommand("exitApp");
        menuItem.addActionListener(this);
        
        
        menu = new JMenu("Edit");
        menuBar.add(menu);
        menuItem = new JMenuItem("Undo");
        menu.add(menuItem);
        menuItem.setActionCommand("undo");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("Redo");
        menu.add(menuItem);
        menuItem.setActionCommand("redo");
        menuItem.addActionListener(this);
        
        
        menu = new JMenu("View");
        menuBar.add(menu);
        
        menuItem = new JMenuItem("Polygon styles");
        menu.add(menuItem);
        menuItem.setActionCommand("showPolygonStyles");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("Edge styles");
        menu.add(menuItem);
        menuItem.setActionCommand("showEdgeStyles");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("Images");
        menu.add(menuItem);
        menuItem.setActionCommand("showImages");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("Options");
        menu.add(menuItem);
        menuItem.setActionCommand("showOptions");
        menuItem.addActionListener(this);
        
        menuItem = new JMenuItem("BeanShell");
        menu.add(menuItem);
        menuItem.setActionCommand("showBeanShellConsole");
        menuItem.addActionListener(this);
        
        scriptMenu = new JMenu("Scripts");
        menuBar.add(scriptMenu);
    }
    
    
    private void buildToolBar() {
        
        JToolBar tb1 = new JToolBar(SwingConstants.HORIZONTAL);
        tb1.setFloatable(false);
        JToolBar tb2 = new JToolBar(SwingConstants.VERTICAL);
        tb2.setFloatable(false);        
        lblZoom = new JLabel("x1");
        lblZoom.setHorizontalAlignment(SwingConstants.CENTER);
        Font f = lblZoom.getFont();
        lblZoom.setFont(f.deriveFont((float)(f.getSize() - 2)));
        toggleGroup = new ButtonGroup();
        
        JToggleButton tb = newToggle("select", "/images/arrow.gif", "Select");
        tb.setSelected(true);
        tb1.add(tb);
        tb1.add(newToggle("editMode", "/images/editobjicon.gif", "Edit objects"));
        tb1.add(newToggle("eraseMode", "/images/eraseicon.gif", "Erasing mode"));
        tb1.add(newToggle("copyMode", "/images/copyicon.gif", "Copy mode"));
        tb1.addSeparator();        
        tb1.add(newButton("undo", "/images/undo.gif", "Undo"));
        tb1.add(newButton("redo", "/images/redo.gif", "Redo"));  
        tb1.addSeparator();        
        tb1.add(makeGridSpinner());
        tb1.add(makeGridToggle());
        tb1.add(makeFastRenderingToggle());
        tb1.add(newButton("zoomIn", "/images/zoominicon.gif", "Zoom in"));
        tb1.add(newButton("zoomOut", "/images/zoomouticon.gif", "Zoom out"));
        tb1.add(lblZoom);
        
        tb2.add(newToggle("newWall", "/images/polyicon.gif", "New wall"));        
        tb2.add(newToggle("newFuel", "/images/fuelicon.gif", "New fuel station"));
        tb2.add(newToggle("newBase", "/images/baseicon.gif", "New base"));
        tb2.add(newToggle("newBall", "/images/ballicon.gif", "New ball"));
        tb2.add(newToggle("newCheckPoint", "/images/checkicon.gif", "New checkpoint"));
        tb2.add(newToggle("newGrav", "/images/gravicon.gif", "New gravitation field"));
        tb2.add(newToggle("newItemConcentrator", "/images/itemconicon.gif", "New item concentrator"));
        tb2.add(newToggle("newAsteroidConcentrator", "/images/asteroidconicon.gif", "New asteroid concentrator"));
        tb2.addSeparator();
        tb2.add(newButton("group", "/images/groupicon.gif", "Group"));
        tb2.add(newButton("ungroup", "/images/ungroupicon.gif", "Ungroup"));
        tb2.add(newButton("regroup", "/images/regroupicon.gif", "Regroup"));
        tb2.addSeparator();
        tb2.add(newButton("makeBallArea", "/images/ballareaicon.gif", "Make ball area"));        
        tb2.add(newButton("makeBallTarget", "/images/balltargeticon.gif", "Make ball target"));
        tb2.add(newButton("makeDecor", "/images/decoricon.gif", "Make decoration"));
        tb2.add(newButton("makeFriction", "/images/fricticon.gif", "Make friction area"));
        tb2.add(newButton("makeTarget", "/images/targeticon.gif", "Make target"));
        tb2.add(newButton("makeCannon", "/images/cannonicon.gif", "Make cannon"));
        tb2.add(newToggle("makeWormhole", "/images/wormicon.gif", "Make wormhole"));
                
        getContentPane().add(tb1, BorderLayout.NORTH);
        getContentPane().add(tb2, BorderLayout.EAST);        
    }
    
    
    private void buildActionMap() {
        ActionMap am = canvas.getActionMap();
        am.put("quickSave", new GuiAction("quickSave"));
        am.put("quickOpen", new GuiAction("quickOpen"));
    }
    
    
    private void buildInputMap() {
        InputMap im = canvas.getInputMap(
        MapCanvas.WHEN_IN_FOCUSED_WINDOW);
        im.put(KeyStroke.getKeyStroke("control S"), "quickSave");
        im.put(KeyStroke.getKeyStroke("control L"), "quickOpen");
        im.put(KeyStroke.getKeyStroke("control Z"), "undo");
    }
    
    
    public void actionPerformed (ActionEvent ae) {
        String cmd = ae.getActionCommand();
        dispatchCommand(cmd);
    }
    
    
    private void dispatchCommand (String cmd) {
        try {
            getClass().getDeclaredMethod(cmd, null).invoke(this, null);
        } catch (NoSuchMethodException nsme) {
            JOptionPane.showMessageDialog
            (this, "Sorry, operation " + cmd + 
            " is not implemented yet", "Error",
            JOptionPane.ERROR_MESSAGE);
        } catch (Exception e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog
            (this, "Unexpected exception: " + e, "Error",
            JOptionPane.ERROR_MESSAGE);
        }
    }
    
    
    private JToggleButton newToggle(String cmd, String name, String toolTip) {
        JToggleButton b =
        new JToggleButton(new ImageIcon(getClass().getResource(name)));
        b.setToolTipText(toolTip);
        b.setActionCommand(cmd);
        b.addActionListener(this);
        b.setPreferredSize(BUTTON_SIZE);
        b.setMaximumSize(BUTTON_SIZE);
        toggleGroup.add(b);
        return b;
    }
    
    
    private JButton newButton(String cmd, String name, String toolTip) {
        JButton b = new JButton(new ImageIcon(getClass().getResource(name)));
        b.setToolTipText(toolTip);
        b.setActionCommand(cmd);
        b.setPreferredSize(BUTTON_SIZE);
        b.setMaximumSize(BUTTON_SIZE);
        b.addActionListener(this);
        return b;
    }
    
    private JSpinner makeGridSpinner() {
        gridSpinner = new JSpinner(
            new SpinnerNumberModel(8, 1, 256, 1) {
                public void setValue(Object value) {
                    super.setValue(value);
                    if (gridToggle.isSelected())
                        canvas.setGrid(getNumber().intValue());
                }
            });
        gridSpinner.setToolTipText("Edit grid size");
        gridSpinner.setPreferredSize(new Dimension(34, 26));
        gridSpinner.setMaximumSize(new Dimension(34, 26));
        return gridSpinner;
    }
        
    private JToggleButton makeGridToggle() {
        JToggleButton b = new JToggleButton(new ImageIcon(
            getClass().getResource("/images/gridicon.gif")));
        b.setToolTipText("Toggle grid");
        b.setActionCommand("grid");
        b.setPreferredSize(BUTTON_SIZE);
        b.setMaximumSize(BUTTON_SIZE);        
        b.addActionListener(this);
        this.gridToggle = b;
        return b;
    }
    
    private JToggleButton makeFastRenderingToggle() {
        JToggleButton b = new JToggleButton(new ImageIcon(
            getClass().getResource("/images/fasticon.gif")));
        b.setToolTipText("Fast rendering mode");
        b.setActionCommand("fast");
        b.addActionListener(this);
        b.setMaximumSize(BUTTON_SIZE);        
        b.setPreferredSize(BUTTON_SIZE);
        return b;
    }
    
    private void fast() {
        canvas.setFastRendering(!canvas.isFastRendering());
        canvas.repaint();
    }
    
    private void grid() {
        if (gridToggle.isSelected()) {
            canvas.setGrid(((Number)gridSpinner.getValue()).intValue());
        } else {
            canvas.setGrid(-1);           
        }
    }
    
    private void setZoom (int zoom) {
        this.zoom = zoom;
        updateScale();
    }
    
    private void zoomIn() {
        zoom++;
        updateScale();
    }
    
    
    private void zoomOut() {
        zoom--;
        updateScale();
    }
    
    
    private void updateScale() {
        float df = canvas.getModel().getDefaultScale();
        canvas.setScale
        ((zoom >= 0) ?
        (df * (zoom + 1)) :
        (df / (-zoom + 1)));
        canvas.repaint();
        
        if (zoom >= 0) {
            lblZoom.setText("x" + (zoom + 1));
        } else {
            lblZoom.setText("x1/" + (-zoom + 1));
        }
    }
    
    private void select() {
        canvas.setMode(MapCanvas.MODE_SELECT);
        canvas.setCanvasEventHandler(null);
        canvas.repaint();
    }
    
    private void newWall() {
        newMapObject(new MapPolygon());
    }
    
    private void makeBallArea() {
        if (canvas.getSelectedObjects().isEmpty()) {
            JOptionPane.showMessageDialog
            (this, "First select the objects that belong to the ball area.", 
            "Info",
            JOptionPane.INFORMATION_MESSAGE);
            return;
        }  
        canvas.makeBallAreaFromSelected();
    }
    
    private void makeBallTarget() {
        if (canvas.getSelectedObjects().isEmpty()) {
            JOptionPane.showMessageDialog
            (this, "First select the objects that belong to the ball target.", 
            "Info",
            JOptionPane.INFORMATION_MESSAGE);
            return;
        }  
        canvas.makeBallTargetFromSelected();
    }
    
    private void makeDecor() {
        if (canvas.getSelectedObjects().isEmpty()) {
            JOptionPane.showMessageDialog
            (this, "First select the objects that belong to the decoration.", 
            "Info",
            JOptionPane.INFORMATION_MESSAGE);
            return;
        }  
        canvas.makeDecorationFromSelected();

    }
    
    private void makeFriction() {
        if (canvas.getSelectedObjects().isEmpty()) {
            JOptionPane.showMessageDialog
            (this, "First select the objects that belong to the friction area.", 
            "Info",
            JOptionPane.INFORMATION_MESSAGE);
            return;
        }  
        canvas.makeFrictionAreaFromSelected();
    }    
    
    private void makeTarget() {
        if (canvas.getSelectedObjects().isEmpty()) {
            JOptionPane.showMessageDialog
            (this, "First select the objects that belong to the target.", 
            "Info",
            JOptionPane.INFORMATION_MESSAGE);
            return;
        }  
        canvas.makeTargetFromSelected();
    }    
    
    private void newFuel() {
        newMapObject(SimpleMapObject.createFuel());
    }
    
    private void newBase() {
        newMapObject(new Base());
    }
    
    private void newBall() {
        newMapObject(new Ball());    
    }
    
    private void makeCannon() {
        if (canvas.getSelectedObjects().isEmpty()) {
            JOptionPane.showMessageDialog
            (this, "First select the objects that belong to the cannon.", 
            "Info",
            JOptionPane.INFORMATION_MESSAGE);
            return;
        }  
        canvas.makeCannonFromSelected();
    }
        
    private void newCheckPoint() {
        newMapObject(SimpleMapObject.createCheck());
    }
    
    private void newItemConcentrator() {
        newMapObject(SimpleMapObject.createItemConcentrator());
    }
    
    private void newAsteroidConcentrator() {
        newMapObject(SimpleMapObject.createAsteroidConcentrator());
    }
    
    private void newGrav() {
        newMapObject(new Grav());
    }
    
    private void makeWormhole() {
        if (canvas.getSelectedObjects().isEmpty()) {
            JOptionPane.showMessageDialog
            (this, "First select the objects that belong to the wormhole.", 
            "Info",
            JOptionPane.INFORMATION_MESSAGE);
            return;
        }  
        canvas.makeWormholeFromSelected();
    }
    
    private void newMapObject(MapObject o) {
        canvas.setMode(MapCanvas.MODE_SELECT);
        canvas.setCanvasEventHandler(o.getCreateHandler(null));        
    }
    
    private void editMode() {
        canvas.setCanvasEventHandler(null);
        canvas.setMode(MapCanvas.MODE_EDIT);
    }
    
    private void eraseMode() {
        canvas.setCanvasEventHandler(null);
        canvas.setMode(MapCanvas.MODE_ERASE);
        canvas.repaint();
    }
    
    private void copyMode() {
        canvas.setCanvasEventHandler(null);
        canvas.setMode(MapCanvas.MODE_COPY);
        canvas.repaint();
    }
    
    private void group() {
        canvas.makeGroupFromSelected();
    }
    
    private void ungroup() {
        canvas.ungroupSelected();
    }
    
    private void regroup() {
        canvas.regroupSelected();
    }
    
    private void newMap() {
        mapFile = null;
        setModel(new MapModel());
        //setZoom(-5);
    }
    
    
    private void openMap() {
        
        JFileChooser fc = new JFileChooser(System.getProperty("user.dir"));
        if (mapFile != null) fc.setSelectedFile(mapFile);
        if (fc.showOpenDialog(this) != JFileChooser.APPROVE_OPTION) return;
        
        File f = fc.getSelectedFile();
        if (f == null) return;
        mapFile = f;
        
        MapModel model = new MapModel();
        try {
            model.load(f.getAbsolutePath());
        } catch (Exception e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog
            (this, "Loading failed: " + e.getMessage(), "Error", 
            JOptionPane.ERROR_MESSAGE);
            return;
        }
        setModel(model);
        //setZoom(-5);
    }
    
    
    private void saveMap() {
        if (!validateMap()) return;
        JFileChooser fc = new JFileChooser(System.getProperty("user.dir"));
        if (mapFile != null) fc.setSelectedFile(mapFile);
        if (fc.showSaveDialog(this) != JFileChooser.APPROVE_OPTION) return;
        
        File f = fc.getSelectedFile();
        if (f == null) return;
        mapFile = f;
        
        try {
            canvas.getModel().save(f);
        } catch (Exception e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog
            (this, "Saving failed: " + e.getMessage(), "Error",
            JOptionPane.ERROR_MESSAGE);
        }
    }
    
    
    private void importXml() {
        JFileChooser fc = new JFileChooser(System.getProperty("user.dir"));
        if (fc.showOpenDialog(this) != JFileChooser.APPROVE_OPTION) return;
        File f = fc.getSelectedFile();
        if (f == null) return;
        
        try {
            
            char buf[] = new char[1024];
            StringBuffer xml = new StringBuffer();
            Reader reader = new FileReader(f);
            try {
                while(true) {
                    int len = reader.read(buf);
                    if (len == -1) break;
                    xml.append(buf, 0, len);
                }
            } finally {
                reader.close();
            }            
            MapModel model = new MapModel();
            model.importXml(xml.toString());
            setModel(model);
            
        } catch (Exception e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog
            (this, "Importing failed: " + e.getMessage(), "Error", 
            JOptionPane.ERROR_MESSAGE);
            return;
        }
    }
    
    private void exportXml() {
        if (!validateMap()) return;
        JFileChooser fc = new JFileChooser(System.getProperty("user.dir"));
        if (fc.showSaveDialog(this) != JFileChooser.APPROVE_OPTION) return;
        
        File f = fc.getSelectedFile();
        if (f == null) return;
        
        try {
            FileWriter writer = new FileWriter(f);
            try {
                writer.write(canvas.getModel().exportXml());
            } finally {
                writer.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog
            (this, "Exporting failed: " + e.getMessage(), "Error", 
            JOptionPane.ERROR_MESSAGE);
            return;
        }            
    }
    
    private void exitApp() {
        System.exit(0);
    }
    
    
    private void showOptions() {
        EditorDialog.show
        (this, 
        new MapOptionEditor(canvas.getModel().options), 
        false, 
        EditorDialog.CLOSE);
    }
    
    
    private void showImages() {
        EditorDialog.show
        (this, 
        new ImageListEditor(canvas.getModel().pixmaps), 
        false,
        EditorDialog.CLOSE);
    }
    
    
    private void showPolygonStyles() {
        EditorDialog.show
        (this, 
        new PolygonStyleManager(canvas), 
        false,
        EditorDialog.CLOSE);
    }
    
    
    private void showEdgeStyles() {
        EditorDialog.show
        (this, 
        new EdgeStyleManager(canvas), 
        false,
        EditorDialog.CLOSE);
    }
    
    
    private void showBeanShellConsole() {
        if (bshConsole == null) {
            try {
                Map vars = new HashMap();
                vars.put("editor", canvas);
                bshConsole = new BshConsole(vars, interpreter.getNameSpace());
            } catch (Throwable t) {
                t.printStackTrace();
                JOptionPane.showMessageDialog
                (this, "Cannot create BeanShell console: " + t.toString(), 
                "Error", JOptionPane.ERROR_MESSAGE);
            }
        }
        if (bshConsole != null)
            bshConsole.setVisible(true);
    }
    
    
    private void quickSave() {
        
        if (mapFile == null) {
            saveMap();
            return;
        }
        try {
            canvas.getModel().save(mapFile);
        } catch (Exception e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog
            (this, "Saving failed: " + e.getMessage(), "Error",
            JOptionPane.ERROR_MESSAGE);
        }
    }
    
    
    private void quickOpen() {
        
        if (mapFile == null) {
            openMap();
            return;
        }
        MapModel model = new MapModel();
        try {
            model.load(mapFile.getAbsolutePath());
        } catch (Exception e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog
            (this, "Loading failed: " + e.getMessage(), "Error", 
            JOptionPane.ERROR_MESSAGE);
            return;
        }
        setModel(model);
    }
    
    private void undo() {
        if (canvas.getUndoManager().canUndo())
            canvas.getUndoManager().undo();
        canvas.repaint();            
    }
    
    private void redo() {
        if (canvas.getUndoManager().canRedo())
            canvas.getUndoManager().redo();
        canvas.repaint();            
    }
    
    
    private boolean validateMap() {
        Object[] error = canvas.getModel().validateModel();
        if (error == null) return true;
        canvas.setSelectedObject((MapObject)error[0]);
        JOptionPane.showMessageDialog
        (this, error[1], "Map validation failed",
        JOptionPane.INFORMATION_MESSAGE);
        canvas.repaint();             
        return false;
    }
    
    
    private class GuiAction extends AbstractAction {
        
        private String cmd;
        
        public GuiAction (String cmd) {
            super();
            this.cmd = cmd;
        }
        
        public void actionPerformed (ActionEvent ae) {
            MainFrame.this.dispatchCommand(cmd);
        }
    }
    
    
    public static void main (String args[]) throws Exception {
        
        try {
            javax.swing.UIManager.setLookAndFeel(
                "com.incors.plaf.kunststoff.KunststoffLookAndFeel");
        } catch (Exception ex) { /* ignore */ }
        MainFrame mf = new MainFrame();
        mf.setVisible(true);
        
        if (args.length == 0) mf.newMap();
        else {
            mf.mapFile = new File(args[0]);
            MapModel model = new MapModel();
            if (args.length > 0) model.load(args[0]);
            mf.setModel(model);
            //mf.setZoom(-5);
        }
    }
}

class BshConsole extends JFrame {
    public BshConsole(Map variables, bsh.NameSpace ns) throws Exception {
        super("BeanShell");
        bsh.util.JConsole jc = new bsh.util.JConsole();
        bsh.Interpreter interpreter = new bsh.Interpreter(jc, ns);
        for (Iterator i = variables.keySet().iterator(); i.hasNext();) {
            String key = (String)i.next();
            interpreter.set(key, variables.get(key));
        }        
        int w = 600; int h = 500;
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds((d.width - w) / 2, (d.height - h) / 2, w, h);
        getContentPane().add(jc);
        setDefaultCloseOperation(HIDE_ON_CLOSE);
        Thread t = new Thread(interpreter);
        t.setDaemon(true);
        t.start();        
    }
}