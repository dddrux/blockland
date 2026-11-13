package demo;
import java.util.*;
public class T3 {
    enum Dir { UP, DOWN, NORTH; Dir opp() { return values()[(ordinal() + 1) % 3]; } }
    public static void main(String[] a) {
        Random r = new Random(42);
        System.out.println(r.nextInt(100) + " " + r.nextInt() + " " + r.nextDouble() + " " + r.nextFloat() + " " + r.nextBoolean() + " " + r.nextLong() + " " + r.nextInt(64) + " " + r.nextInt(1000));
        System.out.println(String.format("%d-%s-%.2f-%5d|%-4s|%x %%", 5, "ab", 3.14159, 42, "z", 255));
        Dir d = Dir.valueOf("DOWN"); System.out.println(d + " " + d.ordinal() + " " + d.opp() + " " + Dir.values().length + " " + d.compareTo(Dir.NORTH));
        switch (d) { case UP: System.out.println("u"); break; case DOWN: System.out.println("d"); break; default: System.out.println("?"); }
        int[] c = {3, 1, 2}; int[] e = c.clone(); e[0] = 9; System.out.println(c[0] + " " + e[0]);
        try { Dir.valueOf("X"); } catch (IllegalArgumentException x) { System.out.println(x.getMessage()); }
        System.out.println(Math.pow(2, 10) + " " + Math.floor(-1.5) + " " + Math.round(2.5) + " " + Math.ceil(1.2) + " " + "abc".compareTo("abd") + " " + "hello".contains("ell") + " " + "a".concat("b"));
    }
}
