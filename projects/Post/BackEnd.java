import java.io.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.*;

public class BackEnd extends ServerResourceAccessible {
    // Use getServerStorageDir() as a default directory
    // TODO sub-program 1 ~ 4 :
    // Create helper funtions to support FrontEnd class
    public String getPassword(String userId){
        try{
            String userPassFile = getServerStorageDir() + userId + "/password.txt";

            File file = new File(userPassFile);
            if (!file.exists()) {
                return null;
            }

            Scanner sc = new Scanner(file);
            String correctPassword = sc.nextLine();

            sc.close();
            return correctPassword;

        }catch(Exception e){
            return null;
        }

    }
    public List<Post> getAllPosts(){
        List<Post> posts = new ArrayList<>();

        File root = new File(getServerStorageDir());
        File[] userDirs = root.listFiles();
        if(userDirs==null) return posts;

        for(File userDir : userDirs){
            String userId = userDir.getName();
            posts.addAll(getUserPosts(userId));
        }
        return posts;
    }
    public int getMaxId(){
        List<Post> allPosts = getAllPosts();
        int maxId = -1;
        File root = new File(getServerStorageDir());
        File[] userDirs = root.listFiles();
        if(userDirs==null)return -1;

        for(File userDir:userDirs){
            File postDir = new File(userDir, "post");
            File[] posts = postDir.listFiles();
            if(posts == null) continue;

            for(File post :posts){
                try{
                    int id = Integer.parseInt(post.getName().replace(".txt", ""));
                    if(id>maxId)maxId=id;
                }catch(Exception e){
                    continue;
                }
            }
        }
        return maxId;
    }
    public void save(String userId, Post post){
        try{
            File userPostFile = new File(getServerStorageDir() + userId + "/post/");
            userPostFile.mkdirs();

            File newPost = new File(getServerStorageDir()+userId+"/post/"+post.getId()+ ".txt");

            BufferedWriter writer = new BufferedWriter(new FileWriter(newPost));
            writer.write(post.getDate());
            writer.newLine();
            writer.write(post.getTitle());
            writer.newLine();
            writer.newLine();
            writer.write(post.getContent());
            writer.close();

        }catch (Exception e){
            return;
        }
    }
    public List<String> getFriends(String userId){
        List<String> friends = new ArrayList<>();
        try{
            File userFriendFile = new File(getServerStorageDir()+userId+"/friend.txt");
            BufferedReader reader = new BufferedReader(new FileReader(userFriendFile));

            String friend;
            while((friend = reader.readLine())!=null){
                friends.add(friend);
            }
            reader.close();
        }catch(Exception e){
            return friends;
        }
        return friends;
    }

    public List<Post> getUserPosts(String userId){
        List<Post> userPosts = new ArrayList<>();
        File userPostFile = new File(getServerStorageDir()+userId+"/post");
        File[] files = userPostFile.listFiles();
        if(files == null) return userPosts;

        for(File f : files){
            try{
                BufferedReader reader = new BufferedReader(new FileReader(f));
                String date = reader.readLine();
                String title = reader.readLine();
                reader.readLine();

                String content = "";
                String line;
                while((line = reader.readLine())!=null){
                    content += line+"\n";
                }
                content = content.trim();

                int id = Integer.parseInt(f.getName().replace(".txt", ""));
                LocalDateTime dt = Post.parseDateTimeString(date, DateTimeFormatter.ofPattern("yyyy/MM/dd HH:mm:ss"));

                Post post= new Post(id, dt, title, content);
                reader.close();
                userPosts.add(post);

            }catch(Exception e){
                continue;
            }
        }
        return userPosts;
    }
    public int keywordCount(Post post, String keyword){
        int count = 0;
        String title = post.getTitle().trim();
        String content = post.getContent().trim();

        if(!title.isEmpty()){
            for(String w :title.split("\\s+")){
                if(w.equals(keyword)) count++;
            }
        }
        if(!content.isEmpty()){
            for(String w:content.split("\\s+")){
                if(w.equals(keyword)) count++;
            }
        }
        return count;
    }
    public int keywordsCount(Post post, String[] keywords){
        int count = 0;
        for(String keyword : keywords){
            count += keywordCount(post, keyword);
        }
        return count;
    }

    public int contentWords(Post post){
        String content = post.getContent().trim();
        if(content.isEmpty())return 0;
        return content.split("\\s+").length;
    }
}
