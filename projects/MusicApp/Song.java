
import java.util.*;

public class Song implements Comparable<Song> {
    private String title;
    private String[] types;

    private HashMap<String, Integer> ratings;

    public Song(String title, String[] types) {
        this.title = title;
        this.types = types;
        this.ratings = new HashMap<>();
    }
    @Override
    public String toString() {
        return title;
    }

    public String getTitle(){
        return title;
    }

    public boolean isSongType(String type){
        boolean isType = false;
        for(int i=0;i<types.length;i++){
            if(types[i].equals(type)){
                isType = true;
                break;
            }
        }
        return isType;
    }

    public void addRating(User user, int rating){
        ratings.put(user.getNickname(), rating);
    }

    public int getRating(User user){
        if(!ratings.containsKey(user.getNickname())){
            return 0;
        }
        return ratings.get(user.getNickname());
    }

    public double getMedianRating(){
        if(ratings.size()==0){
            return 0.0;
        }
        List<Integer> ratingList = new ArrayList<>(ratings.values());
        Collections.sort(ratingList);

        if(ratings.size() % 2==1){
            return ratingList.get(ratings.size()/2);
        }else{
            return ((double)ratingList.get(ratings.size()/2-1)+(double)ratingList.get(ratings.size()/2))/2;
        }
    }

    @Override
    public int compareTo(Song other){
        return this.title.compareTo(other.title);
    }

}
