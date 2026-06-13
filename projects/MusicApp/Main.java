public class Main {
    public static void main(String[] args) {
        MusicApp app = new MusicApp();

        app.addSong("A", new String[]{"pop", "dance"});
        app.addSong("B", new String[]{"pop"});
        app.addSong("C", new String[]{"jazz"});
        app.addSong("D", new String[]{"pop", "jazz"});
        app.addSong("E", new String[]{"dance, jazz"});

        app.addUser("u1");
        app.addUser("u2");

        User u1 = app.findUser("u1");
        User u2 = app.findUser("u2");

        app.rateSong(u1, "A", 5);
        app.rateSong(u1, "B", 4);
        app.rateSong(u1, "C", 3);
        app.rateSong(u1, "D", 5);
        app.rateSong(u1, "E", 2);

        app.rateSong(u2, "A", 1);
        app.rateSong(u2, "B", 5);
        app.rateSong(u2, "D", 3);
    }
}