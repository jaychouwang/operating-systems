import java.util.*;
import java.io.*;

public class Page{

	private String address;
	private char mode;
	private int time;
	private boolean referenced;
	private int counter;
	private int mask = 128;
	private int nextRefTime;
	private ArrayList<Integer> ReferenceTimes; 
	
	public Page(String a, char m){
		ReferenceTimes = new ArrayList<Integer>();
		address = new String(a);
		mode = m;
		referenced = false;
		time = 0;
		counter = 0;
		nextRefTime = 0;
		
	}
	
	public String getAddress(){
		return address;
	}
	
	public void setAddress(String a){
		address = a;
	}
	
	public char getMode(){
		return mode;
	}
	
	public void setMode(char m){
		mode = m;
	}
	
	public int getTime(){
		return time;
	}
	
	public void setTime(int t){
		time = t;
	}
			
	public void removeRefTime(){
		ReferenceTimes.remove(0);
	}
	
	public Integer getNextRefTime(){
		if(!ReferenceTimes.isEmpty()){
			return ReferenceTimes.get(0);
		}
		
		else{
			return null;
		}
	}
	
	public void setNextRefTime(int t){
		ReferenceTimes.add(new Integer(t));
	}
	
	public boolean getRef(){
		return referenced;
	}
	
	public void setRef(boolean r){
		referenced = r;
	}
	
	public int getCounter(){
		return counter;
	}
	
	public void setCounter(){
		counter = (counter >>> 1);
		if(referenced){
			counter = (counter | mask);
		}
	}
	
	public void refreshCounter(){
		counter = 0;
	}
	
	public boolean equals(Object obj){
		Page p = (Page)obj;
		return address.equals(p.getAddress());
	}
	

}