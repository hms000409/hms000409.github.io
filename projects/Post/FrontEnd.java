import java.io.*;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.time.LocalDateTime;

public class FrontEnd {
    private UserInterface ui;
    private BackEnd backend;
    private User user;

    public FrontEnd(UserInterface ui, BackEnd backend) {
        this.ui = ui;
        this.backend = backend;
    }

    public boolean auth(String authInfo){
        // TODO sub-problem 1
        try {
            String[] info = authInfo.split("\n");
            String userId = info[0];
            String password = info[1];

            String correctPassword = backend.getPassword(userId);
            if(correctPassword==null)return false;

            if(password.equalsIgnoreCase(correctPassword)){
                this.user = new User(userId, password);
                return true;
            }else{
                return false;
            }

        } catch (Exception e) {
            return false;
        }
    }

    public void post(Pair<String, String> titleContentPair) {
        // TODO sub-problem 2
        String title = titleContentPair.key;
        String content =titleContentPair.value;

        if(title==null || content ==null){
            return;
        }
        if(this.user==null){
            return;
        }

        LocalDateTime localDateTime = LocalDateTime.now();
        int postId;
        String userId = this.user.id;
        int maxId = backend.getMaxId();
        if(maxId==-1){
            postId = 0;
        }else{
            postId = maxId +1;
        }

        Post newPost = new Post(postId, localDateTime, title, content);
        backend.save(userId, newPost);
    }

    public void recommend(int N){
        // TODO sub-problem 3
        if(this.user==null)return;
        List<String> friends = backend.getFriends(this.user.id);
        List<Post> friendPosts = new LinkedList<>();
        for(String friend:friends){
            List<Post> posts = backend.getUserPosts(friend);
            for(Post post : posts){
                friendPosts.add(post);
            }
        }
        friendPosts.sort((a, b)->b.getDate().compareTo(a.getDate()));
        if(friendPosts.size()>N){
            friendPosts = friendPosts.subList(0, N);
        }
        for(Post friendPost : friendPosts){
            this.ui.println(friendPost);
        }
    }

    public void search(String command) {
        // TODO sub-problem 4
        String[] commandWords = command.trim().split(" ");
        if(commandWords.length < 2) return;
        Set<String> keywords = new HashSet<>();
        for(int i=1;i<commandWords.length;i++){
            keywords.add(commandWords[i]);
        }
        String[] keywordArray = keywords.toArray(new String[0]);

        List<Post> allPosts = backend.getAllPosts();
        List<Post> searched = new ArrayList<>();
        Map<Post, Integer>keywordsCount = new HashMap<>();
        Map<Post, Integer>contentCount = new HashMap<>();

        for(Post post :allPosts){
            if(backend.keywordsCount(post, keywordArray)==0)continue;
            searched.add(post);
            keywordsCount.put(post, backend.keywordsCount(post,keywordArray));
            contentCount.put(post, backend.contentWords(post));
        }

        searched.sort((a, b)->{
            int cmp = keywordsCount.get(b) - keywordsCount.get(a);
            if(cmp!=0)return cmp;
            return contentCount.get(b) - contentCount.get(a);
        });

        if(searched.size() > 10){
            searched = searched.subList(0, 10);
        }
        for(Post post:searched){
            this.ui.println(post.getSummary());
        }
    }

    User getUser(){
        return user;
    }
}
