

import java.util.*;

public class User {
    private String nickname;

    private HashMap<String, Integer> history;
    public User(String nickname) {
        this.nickname = nickname;
        this.history = new HashMap<>();
    }

    public String getNickname() {
        return nickname;
    }

    public void addHistory(Song song){
        if(song!=null){
            if(history.containsKey(song.getTitle())){
                history.put(song.getTitle(), history.get(song.getTitle())+1);
            }else{
                history.put(song.getTitle(), 1);
            }
        }
    }

    public List<String> getHistoryTitle(){
        List<String> titles = new LinkedList<>();
        for(String title : history.keySet()){
            titles.add(title);
        }
        return titles;
    }

    public int getSearchCount(String title){
        if(!history.containsKey(title)){
            return 0;
        }
        return history.get(title);
    }

    @Override
    public String toString() {
        return nickname;
    }
}
