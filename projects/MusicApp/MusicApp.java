
import java.util.*;

public class MusicApp {

    private HashMap<String, Song> songs = new HashMap<>();
    private HashMap<String, User> users = new HashMap<>();

    public boolean addSong(String title, String[] types) {
        // TODO sub-problem 1
        if(title == null || types == null){
            return false;
        }
        if(songs.containsKey(title)){
            return false;
        }
        if(types.length==0){
            return false;
        }
        Song song = new Song(title, types);
        songs.put(title, song);
        return true;
    }

    public boolean addUser(String nickname) {
        // TODO sub-problem 1
        if(nickname == null){
            return false;
        }
        if(users.containsKey(nickname)){
            return false;
        }
        User user = new User(nickname);
        users.put(nickname, user);
        return true;
    }

    public Song findSong(String title) {
        // TODO sub-problem 1
        if(title == null){
            return null;
        }
        if(!songs.containsKey(title)){
            return null;
        }
        return songs.get(title);
    }

    public User findUser(String nickname) {
        // TODO sub-problem 1
        if(nickname == null){
            return null;
        }
        if(!users.containsKey(nickname)){
            return null;
        }
        return users.get(nickname);
    }

    public List<Song> findSongsByTypes(String[] types) {
        // TODO sub-problem 2
        LinkedList<Song> typeSongs = new LinkedList<>();
        if(types == null){
            return typeSongs;
        }
        if(types.length==0){
            return typeSongs;
        }
        for(Song song : songs.values()){
            int match = 1;
            for(int i =0;i<types.length;i++){
                if(!song.isSongType(types[i])){
                    match = 0;
                    break;
                }
            }
            if(match==1){
                typeSongs.add(song);
            }
        }
        typeSongs.sort(null);
        return typeSongs;
    }

    public boolean rateSong(User user, String title, int rating) {
        // TODO sub-problem 3
        if(title==null || findSong(title)==null){
            return false;
        }
        if(user==null || findUser(user.getNickname())==null || findUser(user.getNickname())!=user){
            return false;
        }
        if(rating > 5 || rating<1){
            return false;
        }
        Song song = findSong(title);
        song.addRating(user, rating);
        return true;
    }

    public int getUserRating(User user, String title) {
        // TODO sub-problem 3
        if(user==null||findUser(user.getNickname())==null||findUser(user.getNickname())!=user){
            return -1;
        }
        if(title==null){
            return -1;
        }
        Song song = findSong(title);
        if(song==null){
            return -1;
        }
        return song.getRating(user);
    }

    public List<Song> findUserSongsByTypes(User user, String[] types) {
        // TODO sub-problem 4
        List<Song> songs = new LinkedList<>();
        if(user==null || findUser(user.getNickname())==null||findUser(user.getNickname())!=user){
            return songs;
        }
        songs = findSongsByTypes(types);
        for(int i = 0;i<songs.size();i++){
            user.addHistory(songs.get(i));
        }

        return songs;
    }

    public List<Song> recommend(User user) {
        // TODO sub-problem 4
        if(user==null||findUser(user.getNickname())==null||findUser(user.getNickname())!=user){
            return new LinkedList<>();
        }

        List<String> titles = user.getHistoryTitle();
        List<Song> candidates = new LinkedList<>();

        for(String t :titles){
            candidates.add(findSong(t));
        }

        candidates.sort(new SongComparator(user));

        if(candidates.size()<=3){
            return candidates;
        }
        return candidates.subList(0, 3);
    }
}

class SongComparator implements Comparator<Song>{
    private User user;

    public SongComparator(User user){
        this.user = user;
    }

    @Override
    public int compare(Song a, Song b){
        int cmp = user.getSearchCount(b.getTitle()) - user.getSearchCount(a.getTitle());
        if(cmp != 0) return cmp;

        cmp = Double.compare(b.getMedianRating(), a.getMedianRating());
        if(cmp!=0) return cmp;

        return a.getTitle().compareTo(b.getTitle());
    }
}
