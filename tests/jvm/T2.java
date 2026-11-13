package demo;
import java.util.*;
import java.util.function.*;
public class T2 {
    interface Op { int apply(int a, int b); }
    static int twice(Function<Integer, Integer> f, int x) { return f.apply(f.apply(x)); }
    static String hello(String n) { return "hi " + n; }
    String tag = "inst";
    String wrap(String s) { return tag + ":" + s; }
    public static void main(String[] args) {
        List<String> l = new ArrayList<>(); l.add("pera"); l.add("uva"); l.add("maca"); l.add(1, "kiwi");
        System.out.println(l + " " + l.size() + " " + l.get(2) + " " + l.contains("uva") + " " + l.indexOf("maca"));
        Collections.sort(l); System.out.println(l);
        l.sort((x, y) -> y.length() - x.length() != 0 ? y.length() - x.length() : x.compareTo(y)); System.out.println(l);
        Map<String, Integer> m = new HashMap<>(); m.put("a", 1); m.put("b", 2); m.put("a", 10); System.out.println(m.get("a") + " " + m.get("z") + " " + m.size() + " " + m.containsKey("b") + " " + m.getOrDefault("q", 7));
        int tot = 0; for (Map.Entry<String, Integer> e : m.entrySet()) tot += e.getValue() * e.getKey().length(); System.out.println(tot);
        for (String k : m.keySet()) System.out.print(k + ";"); System.out.println();
        Set<Integer> s = new HashSet<>(); s.add(3); s.add(3); s.add(4); System.out.println(s.size() + " " + s.contains(4));
        Iterator<String> it = l.iterator(); int n = 0; while (it.hasNext()) n += it.next().length(); System.out.println(n);
        Op add = (a, b) -> a + b; int k = 100; Op addk = (a, b) -> a + b + k; System.out.println(add.apply(2, 3) + " " + addk.apply(2, 3));
        System.out.println(twice(x -> x * 3, 5) + " " + twice(x -> x + k, 1));
        Function<String, String> h = T2::hello; System.out.println(h.apply("bob"));
        T2 t = new T2(); Function<String, String> w = t::wrap; System.out.println(w.apply("z"));
        Function<String, Integer> len = String::length; System.out.println(len.apply("abcd"));
        Supplier<List<String>> sup = ArrayList::new; List<String> nl = sup.get(); nl.add("q"); System.out.println(nl);
        Runnable r = () -> System.out.println("run " + k); r.run();
        List<Integer> nums = Arrays.asList(5, 3, 9, 1); Collections.sort(nums); System.out.println(nums);
        nums.forEach(x -> System.out.print(x * 2 + " ")); System.out.println();
        try { l.get(10); } catch (IndexOutOfBoundsException e) { System.out.println("ioobe"); }
    }
}
