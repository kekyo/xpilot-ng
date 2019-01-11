package org.xpilot.jxpmap;

import java.util.Map;

public class ModelObject implements Cloneable, java.io.Serializable {
    public Object deepClone (Map context) {
        Object clone = context.get(this);
        if (clone == null) {
            clone = clone();
            context.put(this, clone);
        }
        return clone;
    }
    
    public Object clone () {
        try {
            return super.clone();
        } catch (CloneNotSupportedException e) {
                e.printStackTrace();
                throw new RuntimeException("cloning failed");
        }
    }
}
