package org.xpilot.jxpmap;

import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.*;
import java.util.*;

public class FindPolygon {

    private static final int ANGLE_0 = 0;
    private static final int ANGLE_45 = 1;
    private static final int ANGLE_90 = 2;
    private static final int ANGLE_135 = 3;
    private static final int ANGLE_180 = 4;
    private static final int ANGLE_225 = 5;
    private static final int ANGLE_270 = 6;
    private static final int ANGLE_315 = 7;
    private static final int ANGLE_360 = 8;
    

	public static Polygon findPolygon(BufferedImage img) {
		Point start = findStart(img);
		if (start == null) return null;
		ArrayList points = smoothen(img, start, merge(connect(img, start)));
		Polygon poly = new Polygon();
        int h = poly.getBounds().height;
		for(Iterator i = points.iterator(); i.hasNext();) {
			int[] p = (int[])i.next();
			poly.addPoint(p[0] * 64, (h - p[1]) * 64);
		}
		return poly;
	}
	
	private static Point findStart(BufferedImage img) {
		for(int y = 0; y < img.getHeight(); y++)
			for(int x = 0; x < img.getWidth(); x++)
				if(fg(img, x, y)) 
					return new Point(x, y);
		return null;
	}
	
	private static ArrayList connect(BufferedImage img, Point start) {
		int x = start.x;
		int y = start.y;
		int dir = 0;
		int dx[] = { 1, 1, 0, -1, -1, -1, 0, 1 };
		int dy[] = { 0, -1, -1, -1, 0, 1, 1, 1 };
		ArrayList points = new ArrayList();
		do
		{
			int dir1 = (dir + 3) % 8;
			int diri = dir1;
			do {
				int x2 = x + dx[diri];
				int y2 = y + dy[diri];
				if (fg(img, x2, y2)) {
					dir = diri;
					x = x2;
					y = y2;
					points.add(new Point(dx[diri], dy[diri]));
					break;
				}
				if (--diri < 0) diri += 8;
			} while(diri != dir1);
		} while(x != start.x || y != start.y);
		return points;
	}
	
	private static ArrayList merge(ArrayList points) {
		ArrayList rv = new ArrayList();
		Iterator i = points.iterator();
		if(!i.hasNext()) return points;
		Point p1 = (Point)i.next();
		while(i.hasNext()) {
			Point p2 = (Point)i.next();
			if(p1 != null) {
				if(parallel(p1, p2)) {
					p1.x += p2.x;
					p1.y += p2.y;
					if(!i.hasNext()) rv.add(p1);
				} else {
					rv.add(p1);
					p1 = p2;
				}
			}
		}
		return rv;
	}
	
	private static boolean parallel(Point p1, Point p2) {
		return sign(p1.x) == sign(p2.x) && p1.x * p2.y == p2.x * p1.y;
	}
	
	private static int infnorm(Point p) {
		return Math.max(Math.abs(p.x), Math.abs(p.y));
	}
	
	private static ArrayList smoothen(
	BufferedImage img, Point start, ArrayList points) {
		ArrayList list = new ArrayList();
		int x = start.x;
		int y = start.y;
		list.add(new int[] { x, y, 0 });
		for(int i = 0; i < points.size(); i++) {
			Point p1 = (Point)points.get(i);
			Point p2 = (Point)points.get((i + 1) % points.size());
			Point p3 = (Point)points.get((i + 2) % points.size());
			x += p1.x;
			y += p1.y;
			if (infnorm(p1) > 1) {
				if (infnorm(p2) == 1 && infnorm(p3) > 1) {
					int d1 = dir(p1);
					int d3 = dir(p3);
					if ((d1 + 2) % 8 == d3) {
						if (p1.x != 0) x += sign(p1.x);
						if (p1.y != 0) y += sign(p1.y);
						if (p3.x != 0) p3.x += sign(p3.x);
						if (p3.y != 0) p3.y += sign(p3.y);
						i++;
						list.add(new int[] { x, y, 1 });
						continue;
					}
				} else if (infnorm(p2) > 1) {
					int d1 = dir(p1);
					int d2 = dir(p2);
					if ((d1 + 1) % 8 == d2) {
						list.add(new int[] { x, y, 1 });
						continue;
					}
				}
			}
            list.add(new int[] { x, y, 0 });
        }
			
        Point max = new Point(img.getWidth() - 1, img.getHeight() - 1);
        for(Iterator i = list.iterator(); i.hasNext();) {
            int p[] = (int[])i.next();
            int rm = 0;
            if (p[2] == 0) {
                if (p[0] == 0 || !fg(img, p[0] - 1, p[1])) rm++;
                if (p[0] == max.x || !fg(img, p[0] + 1, p[1])) rm++;
                if (p[1] == 0 || !fg(img, p[0], p[1] - 1)) rm++;
                if (p[1] == max.y || !fg(img, p[0], p[1] + 1)) rm++;
            }
            if(rm == 1) i.remove();
        }
			
        return list;
    }
		
    private static int dir(Point p) {
        int x = p.x <= 0 ? (p.x != 0 ? -1 : 0) : 1;
        int y = p.y <= 0 ? (p.y != 0 ? 1 : 0) : -1;
        if (x == 0 && y == 1) return ANGLE_90;
        if (x == 0 && y == -1) return ANGLE_270;
        if (x == 1 && y == 0) return ANGLE_0;
        if (x == 1 && y == 1) return ANGLE_45;
        if (x == 1 && y == -1) return ANGLE_315;
        if (x == -1 && y == 0) return ANGLE_180;
        if (x == -1 && y == 1) return ANGLE_135;
        if (x == -1 && y == -1) return ANGLE_225;
        throw new IllegalArgumentException(p.toString());
    }
    
    private static int sign(int i) {
        return i < 0 ? -1 : 1;
    }
    
    private static boolean fg(BufferedImage img, int x, int y) {
        if (x < 0 || x >= img.getWidth())
            return false;
        if (y < 0 || y >= img.getHeight())
            return false;
        return (img.getRGB(x, y) & 0xffffff) != 0;
    }		
}
