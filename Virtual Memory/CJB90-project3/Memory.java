import java.util.*;
import java.io.*;

public class Memory{

    private ArrayList<Page> Frames;
	private int max;
	
	public Memory(int n){
		Frames = new ArrayList<Page>(n);	
		max = n;
	}

	public boolean add(Page p){
		if(Frames.contains(p)){
			
			int i = Frames.indexOf(p);
				
			if(i >= 0){
				Page updatePage = Frames.get(i);
				updatePage.setRef(true);
				updatePage.setTime(p.getTime());
				if(p.getMode() == 'W'){
					updatePage.setMode('W');
				}
			}
						
			return true;
		}
		
		else if(isFull()){
			return false;
		}
		
		else{
			Frames.add(p);
			return true;
		}
	
	}
	
	public boolean add(int i, Page p){
		Frames.add(i,p);
		return true;
	}
	
	public boolean remove(Page p){
		return Frames.remove(p);
	}
	
	public boolean remove(int i){
		Frames.remove(i);
		return true;
	}
	
	public Page get(int i){
		return Frames.get(i);
	}
	
	public int size(){
		return Frames.size();
	}
	
	private boolean isFull(){
		return (Frames.size() == max);
	}

}