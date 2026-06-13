import java.util.*;

public class Diary {

    public List<DiaryEntry> diaryEntries;
    public Map<String, Set<Integer>> searchMap;


    public Diary() {
        //TODO
        // Practice 2
        diaryEntries = new LinkedList<>();
        searchMap = new HashMap<>();
    }

    public void createEntry() {
        //TODO
        // Practice 1 - Create a diary entry
        // Practice 2 - Store the created entry in a file

        String title = DiaryUI.input("title: ");
        String content = DiaryUI.input("content: ");

        // 1. Create a DiaryEntry instance and add to diaryEntries.
        DiaryEntry entry = new DiaryEntry(title, content); // <-- You SHOULD create a new instance here.

        // 2. Add the instance to diaryEntries.
        diaryEntries.add(entry);

        // 3. Implement Set<String> for searchMap.
        Set<String> keywords = new HashSet<>();
        for(String s : title.split(" ")){
            keywords.add(s);
        }
        for(String s:content.split(" ")){
            keywords.add(s);
        }

        // 4. Add the Set<String> to searchMap.
        for(String key : keywords){
            if(searchMap.keySet().contains(key)){
                searchMap.get(key).add(entry.getID());
            }else{
                searchMap.put(key, new HashSet<Integer>());
                searchMap.get(key).add(entry.getID());
            }
        }

        DiaryUI.print("The entry is saved.");
    }




    public void listEntries() {
        //TODO
        // Practice 1 - List all the entries - sorted in created time by descending order
        // Practice 2 - Your list should contain previously stored files
        if(diaryEntries.isEmpty())
        // 1. Print out all of "DiaryEntry" in diaryEntries using getShortString().
        diaryEntries.sort(Comparator.comparing(DiaryEntry::getDateTimeString));
        for(DiaryEntry entry : diaryEntries){
            DiaryUI.print(entry.getShortString());
        }

    }


    private DiaryEntry findEntry(int id){

        //TODO
        // Practice 1 - Find a DiaryEntry instance using an unique id.

        // 1. Find the DiaryEntry instance in diaryEntries with the given id.
        Iterator<DiaryEntry> iterator = diaryEntries.iterator();
        while(iterator.hasNext()){
            DiaryEntry entry = iterator.next();
            if(entry.getID() == id){
                return entry;
            }
        }
        // If there is no matching instance, return null.
        return null;
    }



    public void readEntry(int id) {
        //TODO
        // Practice 1 - Read the entry of given id
        // Practice 2 - Your read should contain previously stored files

        // 1. Find a DiaryEntry instance using findEntry(int id) above.
        DiaryEntry entry = findEntry(id);


        // 2. Print out that instance.
        if(entry==null){
            DiaryUI.print("There is no entry with id "+id+".");
        }else{
            DiaryUI.print(entry.getFullString());
        }
        // You SHOULD print an alert message if there is no matching instance.

    }

    public void deleteEntry(int id) {
        //TODO
        // Practice 1 - Delete the entry of given id
        // Practice 2 - Delete the file of the entry

        // 1. Find a DiaryEntry instance using findEntry(int id) above.
        DiaryEntry entry = findEntry(id);

        // 2. Delete the DiaryEntry instance from diaryEntries and searchMap.
        // You SHOULD print an alert message if there is no matching instance.
        // And you SHOULD print a success message if deletion is done.
        if(entry==null){
            DiaryUI.print("There is no entry with id "+id+".");
        }else{
            diaryEntries.remove(entry);
            for(Set<Integer> idSet : searchMap.values()){
                idSet.remove(id);
            }
            DiaryUI.print("Entry "+id+" is removed.");
        }
    }

    public void searchEntry(String keyword) {
        //TODO
        // Practice 1 - Search and print all the entries containing given keyword
        // Practice 2 - Your search should contain previously stored files

        Set<Integer> idSet = searchMap.get(keyword);
        if(idSet==null || idSet.size()==0){
            DiaryUI.print("There is no entry that contains \""+keyword+"\".");
        }else{
            Iterator<Integer> iterator = idSet.iterator();
            DiaryEntry entry = findEntry(iterator.next());
            DiaryUI.print(entry.getFullString());
            while(iterator.hasNext()){
                entry = findEntry(iterator.next());
                DiaryUI.print("\n"+entry.getFullString());
            }
        }
        // You SHOULD print an alert message if there is no matching instance.
        // And you SHOULD print information of instances using getFullString().
    }
}
