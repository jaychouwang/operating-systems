
void cs1550_init(){
	
	char* mode = "r+b";
	int result;
	char allocationTable[512];
	struct cs1550_root_directory* root;

	FILE* disk = fopen(".disk", mode);

	root = malloc(sizeof(cs1550_root_directory));
	result = fread(root,512,1,disk);
	
	//if(result != 512){
	//	root->nDirectories = 0;
	//	mode* = "r+b";
	//	error = fwrite(buffer , sizeof(cs1550_root_directory), 1, disk);
	//}
	
	fseek (disk, 512, SEEK_SET);
	result = fread(allocationTable,512,1,disk);
	
	fclose(disk);
	
	allocationTable[0] = 1;
	allocationTable[1] = 1;
}

int cs1550_read_disk(int block, void* buf){
	char* mode = "r+b";
	FILE* disk = fopen(".disk", mode);
	buf = malloc(sizeof(cs1550_directory_entry));
	fseek (disk, (512*block), SEEK_SET);
	return fread(buf,512,1,disk);

}

int cs1550_write_disk(int block, void* buf){
	char* mode = "r+b";
	FILE* disk = fopen(".disk", mode);
	fseek (disk, (512*block), SEEK_SET);
	return fwrite(buf, 512, 1, disk);
}