package demo;
interface Shape { double area(); String name(); }
abstract class Base implements Shape { protected int id; static int count = 0; Base() { id = ++count; } public String name() { return "base" + id; } }
class Sq extends Base { double s; Sq(double s) { this.s = s; } public double area() { return s * s; } }
class Ci extends Base { double r; Ci(double r) { this.r = r; } public double area() { return 3.0 * r * r; } public String name() { return "circle" + id; } }
class MyEx extends RuntimeException { int code; MyEx(String m, int c) { super(m); code = c; } }
public class T {
    static final String K = "const";
    static int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
    static long fact(int n) { long r = 1; for (int i = 2; i <= n; i++) r *= i; return r; }
    static int sw(int x) { switch (x) { case 1: return 10; case 2: return 20; case 5: return 50; default: return -1; } }
    static int sw2(int x) { switch (x) { case 100: return 1; case 2000: return 2; case 30000: return 3; default: return 0; } }
    static void thrower(int c) { if (c > 2) throw new MyEx("boom", c); }
    public static void main(String[] args) {
        System.out.println(fib(20)); System.out.println(fact(15));
        int[] a = new int[10]; for (int i = 0; i < a.length; i++) a[i] = i * i; int sum = 0; for (int v : a) sum += v; System.out.println(sum);
        Shape[] sh = { new Sq(2.5), new Ci(2), new Sq(1) };
        double tot = 0; for (Shape s : sh) { tot += s.area(); System.out.println(s.name() + " " + s.area()); } System.out.println(tot);
        StringBuilder sb = new StringBuilder(); for (int i = 0; i < 5; i++) sb.append(i).append(','); sb.append("x").append(1.5f).append(2L).append(true).append('c'); System.out.println(sb.toString());
        try { thrower(1); thrower(5); System.out.println("nao"); } catch (MyEx e) { System.out.println("pegou " + e.getMessage() + " " + e.code); } finally { System.out.println("fin"); }
        try { int z = 0; System.out.println(5 / z); } catch (ArithmeticException e) { System.out.println("arit " + e.getMessage()); }
        try { int[] b = new int[2]; b[2] = 1; } catch (RuntimeException e) { System.out.println("rt " + e.getMessage()); }
        try { Object o = null; o.hashCode(); } catch (NullPointerException e) { System.out.println("npe"); }
        System.out.println(sw(1) + " " + sw(2) + " " + sw(5) + " " + sw(7) + " " + sw2(2000) + " " + sw2(30000) + " " + sw2(3));
        System.out.println(K + K.length() + K.charAt(1) + K.substring(2) + K.indexOf("st") + K.toUpperCase());
        long big = 1L << 40; System.out.println(big + " " + (big >>> 3) + " " + (int) big + " " + (-17 % 5) + " " + (-17 / 5) + " " + (7 >> 1) + " " + (-7 >>> 28));
        double d = 10.0 / 3; System.out.println(d + " " + (float) d + " " + (int) 3.99 + " " + (long) -2.5 + " " + Math.sqrt(2) + " " + Math.max(3, 9) + " " + Math.abs(-4.5));
        Integer boxed = 42; int un = boxed; System.out.println(boxed + un); System.out.println(Integer.MAX_VALUE + 1);
        char ch = 'a'; ch += 2; byte by = (byte) 200; short st = (short) 70000; System.out.println(ch + " " + by + " " + st);
        String s1 = "ab", s2 = "a" + "b"; System.out.println(s1.equals(s2) + " " + (s1.hashCode() == s2.hashCode()));
        int[][] m = new int[3][4]; m[1][2] = 7; System.out.println(m[1][2] + m.length + m[0].length);
        long acc = 0; for (int i = 0; i < 300000; i++) acc += i % 7 * (i & 3); System.out.println(acc);
    }
}
