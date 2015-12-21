import java.util.*;
import java.io.*;

public class vmsim{

	static int numFrames;
	static int numPageFaults;
	static int numRefs;
	static int numWrites;
	static int refresh;
	static int tau;
	static int counter;
	static String traceFile;
	static String algorithm;
	static Page[] References;
	static Hashtable<String, Page> ReferenceTimes;
	static Memory RAM;
	static Scanner refFile;


public static void main(String [] args){

	counter = 0;
	int error = 0;
	
	if(args.length > 0){
		
		if(args[0].toLowerCase().equals("-n")){
			numFrames = Integer.parseInt(args[1]);
		}
		
		else {error = 1;}	
			
		if(args[2].toLowerCase().equals("-a")){
			algorithm = new String(args[3].toLowerCase());
		}
		
		else {error = 2;}
		
				
		if(algorithm.equals("aging") || algorithm.equals("work")){
			
			if(args[4].toLowerCase().equals("-r")){
				refresh = Integer.parseInt(args[5]);
			}
			
			else {error = 3;}	
		
							
			if(algorithm.equals("work")){
				if(args[6].toLowerCase().equals("-t")){
					tau = Integer.parseInt(args[7]);
					traceFile = new String(args[8]);
				}
				
				else {error = 4;}
				
			}
			
			else{
				traceFile = new String(args[6]);
			}
		}
		
		else{
			traceFile = new String(args[4]);
		}
	
	}
		
	else{
		System.out.println("Argument Error: No arguments");
		System.exit(0);
	}
	
	if(error > 0){
	
		switch (error) {
            case 1:  System.out.println("Argument Error: Number of frames needed");
                     break;
            case 2:  System.out.println("Argument Error: Algorithm option needed");
                     break;
            case 3:  System.out.println("Argument Error: Refresh time needed");
                     break;
            case 4:  System.out.println("Argument Error: Tau needed");
                     break;
		}
	
		System.exit(0);	
	
	}
	
	try{
		refFile = new Scanner(new File(traceFile));
	}
	
	catch(FileNotFoundException E){
		System.out.println("Argument Error: Invalid File");
		System.exit(0);
	}
	
    List<Page> tempReferences = new ArrayList<Page>();
    ReferenceTimes = new Hashtable<String, Page>();
	String tempStr;
	char tempChar;
	Page tempPage;
	
    while(refFile.hasNext()) {
      tempStr = refFile.nextLine();
      tempChar = tempStr.charAt(9);
      tempStr = tempStr.substring(0,8);
	  tempPage = new Page(tempStr,tempChar);
      tempReferences.add(tempPage);
      
      if(algorithm.equals("opt")){
      	if(ReferenceTimes.containsKey(tempPage.getAddress())){
      		Page tempPage2 = ReferenceTimes.get(tempPage.getAddress());
      		tempPage2.setNextRefTime(counter);
      	}
      	else{
      		tempPage.setNextRefTime(counter);
      		ReferenceTimes.put(tempPage.getAddress(),tempPage);
      	}
      }
      counter++;
    }
    
    refFile.close();

    References = tempReferences.toArray(new Page[0]);
    RAM = new Memory(numFrames);
    numPageFaults = numFrames;
	numRefs = 0;
	numWrites = 0;
	
	if(algorithm.equals("opt")){
		Optimal();
	}
	
	else if(algorithm.equals("clock")){
		Clock();
	}
	
	else if(algorithm.equals("aging")){
		Aging();
	
	}
	
	else{
		WSClock();
	}
	
	System.out.println(algorithm);
	System.out.println("Number of frames " + numFrames);
	System.out.println("Total memory accesses: " + numRefs);
	System.out.println("Total page faults: " + numPageFaults);
	System.out.println("Total writes to disk: " + numWrites);
	
}

public static void Optimal(){

	int line = 0;
	Page currentPage;
	Page tempPage;
	Page replacement;
	
	while(line < counter){
	
		currentPage = References[line];
	
		if(!RAM.add(currentPage)){

			numPageFaults++;
			int maxRefValue = 0;
			int maxRefIndex = 0;
			
			for(int i = 0; i < numFrames; i++){
			
				tempPage = ReferenceTimes.get(RAM.get(i).getAddress());
				Integer nextRefTime = tempPage.getNextRefTime();
				
				while(nextRefTime != null && nextRefTime.intValue() < line){
					tempPage.removeRefTime();
					nextRefTime = tempPage.getNextRefTime();
				}
				
				if(nextRefTime ==  null){
					maxRefIndex = i;
					break;
				}
				
				else if(nextRefTime > maxRefValue){
					maxRefValue = nextRefTime;
					maxRefIndex = i;
				}
				
			}
			
			replacement = RAM.get(maxRefIndex);
			
			if(replacement.getMode() == 'W'){
					numWrites++;
			}
			
			RAM.remove(maxRefIndex);
			RAM.add(maxRefIndex, currentPage);
		}
		
	
		line++;
		numRefs++;
	}
	
}

public static void Clock(){

	int pointer = 0;
	int line = 0;
	Page currentPage;
	
	while(line < counter){
		
		currentPage = References[line];
		
		if(!RAM.add(currentPage)){
		
			boolean found = false;
			Page replacement;
			numPageFaults++;
			
			do{
			
			replacement = RAM.get(pointer);
			if(replacement.getRef()){
				replacement.setRef(false);
				pointer = ((pointer + 1) % numFrames);
			}
			
			else{
				if(replacement.getMode() == 'W'){
					numWrites++;
				}
				RAM.remove(pointer);
				RAM.add(pointer, currentPage);
				found = true;
				pointer = ((pointer + 1) % numFrames);
			}
			
			}while(!found);
			
			
		}
		
		line++;
		numRefs++;
	}
	
}

public static void Aging(){
    
	int line = 0;
	Page currentPage;
	
	int startTime = 0;
	int endTime = 0;
	
	while(line < counter){
		
		Page updateEntry;
		if((endTime - startTime) == refresh){
		
			for(int i = 0; i < RAM.size(); i++){
				updateEntry = RAM.get(i);
				updateEntry.setCounter();
				updateEntry.setRef(false);
			}
			startTime = line;
		}
		
		currentPage = References[line];
		
		if(!RAM.add(currentPage)){
		
			int minCounter = Integer.MAX_VALUE;
			int minIndex = 0;
			Page replacement;
			numPageFaults++;
			
			int tempCounter;
			for(int i = 0; i < numFrames; i++){
				replacement = RAM.get(i);
				tempCounter = replacement.getCounter();
				
				if(tempCounter < minCounter){
					minCounter = tempCounter;
					minIndex = i;
				}
				
			}
			
			replacement = RAM.get(minIndex);
			if(replacement.getMode() == 'W'){
					numWrites++;
			}
			
			RAM.remove(minIndex);
			RAM.add(minIndex, currentPage);
		}
		
		endTime = line;
		line++;
		numRefs++;
	}

}

public static void WSClock(){

	int pointer = 0;
	int line = 0;
	int startTime = 0;
	int endTime = 0;
	Page currentPage;
	
	while(line < counter){
	
		Page updateEntry;
		if((endTime - startTime) >= refresh){
		
			for(int i = 0; i < RAM.size(); i++){
				updateEntry = RAM.get(i);
				updateEntry.setCounter();
				updateEntry.setRef(false);
				updateEntry.setTime(line);
			}
			startTime = line;
		}
		
		
		currentPage = References[line];
		currentPage.setTime(line);
		
		if(!RAM.add(currentPage)){
		
			boolean found = false;
			int beginPointer = pointer;
			int[] pageTimes = new int[numFrames];
			Page replacement;
			numPageFaults++;
			
			do{
			replacement = RAM.get(pointer);
			
			if(!replacement.getRef() && replacement.getMode() != 'W'){
				RAM.remove(pointer);
				RAM.add(pointer, currentPage);
				pointer = ((pointer + 1) % numFrames);
				found = true;

			}
			
			else if(!replacement.getRef() && replacement.getMode() == 'W' && (line - replacement.getTime() > tau)){
				numWrites++;
				replacement.setMode('R');
				pageTimes[pointer] = replacement.getTime();
				pointer = ((pointer + 1) % numFrames);
			}
			
			else{
				pageTimes[pointer] = replacement.getTime();
				pointer = ((pointer + 1) % numFrames);
			}
			
			
			if(pointer == beginPointer && !found){
				
				int oldestPage = getMinIndex(pageTimes);
				replacement = RAM.get(oldestPage);

				if(replacement.getMode() == 'W'){
					numWrites++;
				}
			
				RAM.remove(oldestPage);
				RAM.add(oldestPage, currentPage);
				found = true;

			}
			
			}while(!found);
			
			
		}
		
		endTime = line;
		line++;
		numRefs++;
	}
	
}

private static int getMinIndex(int[] a){  
  int minIndex = 0;  
  int minValue = a[0];
  for(int i=1; i < a.length; i++){  
    if(a[i] < minValue){  
      minIndex = i; 
      minValue = a[i];
    }  
  }  
  return minIndex;  
} 

}