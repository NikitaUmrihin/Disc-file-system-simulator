#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>

using namespace std;

#define DISK_SIZE 256

// ============================================================================
//  .   Notes:
//  .        Some behaviour wasn't defined by the exercise, so I implemented
//  .        it with my logic and understanding of the material....
//  .        Also, I didn't break the code up into header files, 
//  .        since it isn't that long... 
//  .        ( seems pointless to have 6 files for this project )
// ============================================================================


// ============================================================================

class FsFile {
    int file_size;
    int block_in_use;
    int index_block;
    int block_size;
    
    // Constructor
    public:
    FsFile(int _block_size) 
    {
        file_size = 0;
        block_in_use = 0;
        block_size = _block_size;
        index_block = -1;
    }

    // getters + setters
    int get_file_size()
    {
        return file_size;
    }
    
    int get_actual_file_size()
    {
        return file_size+block_size;
    }
    
    void set_file_size(int num)
    {
        file_size = num;
    }
    
    
    int get_index_block()
    {
        return index_block;
    }
    
    void set_index_block(int i)
    {
        index_block = i;
    }

};

// ============================================================================

class FileDescriptor {
    string file_name;
    FsFile* fs_file;
    bool inUse;
    
    // Constructor
    public:
    FileDescriptor(string FileName, FsFile* fsi) 
    {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
    }
    
    // setters + getters
    string getFileName() 
    {
        return file_name;
    }

    FsFile* get_fs_file()
    {
        return fs_file;
    }
    
    bool get_in_use()
    {
        return inUse;
    }
    
    void set_in_use(bool b)
    {
        inUse = b;
    }


};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

// ============================================================================

class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;
    int blockSize;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
	//              or not.  (i.e. if BitVector[0] == 1 , means that the
	//             first block is occupied.
    int BitVectorSize;
    int *BitVector;
    // (5) MainDir --
    // Structure that links the file name to its FsFile
    vector<FileDescriptor*> MainDir;

    // (6) OpenFileDescriptors --
    // when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector<FileDescriptor*> OpenFileDescriptors;

    
    // ------------------------------------------------------------------------
   
    // Constructor
    public:
    fsDisk() 
    {
        sim_disk_fd = fopen(DISK_SIM_FILE , "r+");
        assert(sim_disk_fd);
        
        for (int i=0; i < DISK_SIZE ; i++) 
        {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        
        fflush(sim_disk_fd);
        is_formated = false;
    }
	// ------------------------------------------------------------------------
  
    // Destructor
    ~fsDisk()
    {
        for(int i=0; i<MainDir.size(); i++)
        {
            if(MainDir[i]!=NULL)
            {
                delete(MainDir[i]->get_fs_file());
                delete(MainDir[i]);
            }
        }
        free(BitVector);
        fclose(sim_disk_fd);
    }
    
    // ------------------------------------------------------------------------
    
    // lists all open files, and disc content
    void listAll() 
    {
        int i = 0;    
        
        for (int i = 0; i < OpenFileDescriptors.size(); i++) 
        {
            if(OpenFileDescriptors[i]!=NULL)
            cout << "index: " << i << ": FileName: " << MainDir[i]->getFileName()  <<  " , isInUse: " << MainDir[i]->get_in_use() << endl;
        }
        
        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++)
        {
            if(i%blockSize==0)
            cout<<" | ";
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread(&bufy, 1, 1, sim_disk_fd);
            cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    
    // Format disc to specific block size, if disc was already formatted,
    // all files are deleted , and a new block size is assigned
    void fsFormat( int blockSize =4 ) 
    {
        //  if disc already formatted
        if(is_formated == true)
        {
            // delete all files from disc
            for(int i=0; i<MainDir.size(); i++)
            {
                if(MainDir[i]!=NULL)
                    DelFile(MainDir[i]->getFileName());
                    
            }
            free(BitVector);
        }
        
        // initialize BitVector
        BitVectorSize = DISK_SIZE/blockSize;
        cout<<"Formatting Disc...\nNumber of blocks: "<<BitVectorSize<<"\n";
        BitVector =(int*) malloc(BitVectorSize* sizeof(int));
        
        for(int i=0; i<BitVectorSize; i++)
            BitVector[i] = 0;
        
        // update block size , and format-flag
        this->blockSize = blockSize;
        is_formated = true;
        
    }

    // ------------------------------------------------------------------------

    int CreateFile(string fileName) 
    {
        if(is_formated  == false)
            return -1;
        FsFile *newFile = new FsFile(blockSize);
        
        // check duplicate name of file
        for(int i=0; i<MainDir.size(); i++)
        {
            if (MainDir[i]!=NULL && MainDir[i]->getFileName() == fileName)
            {
                cout<<fileName<<" already exists, use different name..."<<endl;
                delete(newFile);
                return -1;
            }
        }
        
        // go through BitVector
        for(int i=0; i<BitVectorSize; i++)
        {
            // found an empty block
            if(BitVector[i] == 0)
            {
                // update BitVector and index block
                BitVector[i] = 1;
                newFile->set_index_block(i);
                BitVector[i]=1;
                break;
            }
            
        }
        // if an index block wasn't assigned to new file, the disc is full
        if(newFile->get_index_block() == -1)
        {
            cout<<"ERROR !!! no space for new file..."<<endl;
            delete(newFile);
            return -1;
        }
        
        
        FileDescriptor *fd = new FileDescriptor(fileName, newFile);
        
        // add new file to MainDir
        for(int i=0; i<MainDir.size(); i++)
        {
            if(MainDir[i]==NULL)
            {
                MainDir[i]=fd;
                OpenFileDescriptors[i]=fd;
                return i;
            }
        }
        
        MainDir.push_back(fd);
        OpenFileDescriptors.push_back(fd);
        
        return MainDir.size()-1;
        
        
    }

    // ------------------------------------------------------------------------
    
    int OpenFile(string fileName) 
    {
        
        // check Main Dir
        int i;
        for (i = 0; i < MainDir.size(); i++)
        {
            //  find matching file name
            if(MainDir[i]!=NULL && MainDir[i]->getFileName()==fileName)
            {
                if(MainDir[i]->get_in_use() == true)
                {
                    cout<<fileName<<" already open..."<<endl;
                    return -1;
                }
                else 
                {
                    // open file , move from MainDir to OpenFileDescriptors
                    MainDir[i]->set_in_use(true);
                    OpenFileDescriptors[i]=MainDir[i];
                    return i;
                    
                }
            }
            
            if(i==MainDir.size())
                return -1;
        }
        cout<<"no such file \""<<fileName<<"\""<<endl;
        return -1;
    }  

    // ------------------------------------------------------------------------
    
    string CloseFile(int fd) 
    {
        // go through OpenFileDescriptors
        for (int i = 0; i < OpenFileDescriptors.size(); i++)
        {
            // find our file
            if( i == fd && OpenFileDescriptors[i]!=NULL)
            {
                // if file is open
                if(OpenFileDescriptors[i]->get_in_use() == true)
                {
                    // close file
                    OpenFileDescriptors[i]->set_in_use(false);
                    OpenFileDescriptors[i] = NULL;
                    return to_string(i);
                }
            }
            
        }
        // check if our file is already closed
        for (int i = 0; i < MainDir.size(); i++)
        {
            if( i == fd )
            {
                if(MainDir[i]->get_in_use() == false)
                {
                    cout<<fd<<" - "<<MainDir[i]->getFileName()<<" is already closed :)"<<endl;
                    return "-1";
                }
            }
            
        }
        // if file isn't open or closed , no such file...
        cout<<"no such file \""<<fd<<"\""<<endl;
        return "-1";
    }
    
    // ------------------------------------------------------------------------
    
    // Write buf to file of given fd, returns 0 on success -1 when failed
    int WriteToFile(int fd, char *buf, int len ) 
    {
        // disc was NOT formatted
        if(is_formated == false)
            return -1;
        
        // fd too big or negative
        if(fd > OpenFileDescriptors.size() || fd < 0)
            return -1;
        
        // no such file
        if(MainDir[fd]==NULL)
            return -1;
        
        // file is closed
        if(MainDir[fd]->get_in_use()==false)
            return -1;
        
        // length of string is too big for our file
        if(len > blockSize*blockSize)
            return -1;
        
        if(OpenFileDescriptors[fd]->get_fs_file()->get_file_size() + len > blockSize*blockSize)
            return -1;

        // initialize variables
        int block = 0;
        int index = 0;
        int count = 0;
        int check = 0;
        
        // check = how many chars we have already written to disc    
        while(check<len)
        {   
            check = 0;
            len = strlen(buf);
            // if we need a new block ! (all previously used blocks are full !)
            if(OpenFileDescriptors[fd]->get_fs_file()->get_file_size()%blockSize == 0)
            {
                count++;
                // go through BitVector
                for(block=0; block<BitVectorSize; block++)
                {
                    // found empty block
                    if(BitVector[block]==0)
                    {
                        // go to correct location in disc
                        for(index = block*blockSize; index<block*blockSize+blockSize; index++)
                        {
                            fseek(sim_disk_fd, index, SEEK_SET);
                            for( ; check <blockSize; check++)
                            {
                                // write to disc
                                if(check<len)
                                {
                                    fwrite (buf+check , sizeof(char), sizeof(char), sim_disk_fd);
                                }
                            }
                        }
                        // if we need another block
                        if(len>blockSize- OpenFileDescriptors[fd]->get_fs_file()->get_file_size()%blockSize)
                        {
                            // update buf to represent the chars we did NOT write to disc
                            strncpy(buf,&buf[check],len-check);
                            buf[len-check]='\0';
                        }
                        // update BitVector
                        BitVector[(index-1)/blockSize]=1;
                        break;
                    }
                }
                // no free block was found ...
                if (block==BitVectorSize)
                    return -1;
                    
                // update index block
                for(int j=OpenFileDescriptors[fd]->get_fs_file()->get_index_block()*blockSize; j<OpenFileDescriptors[fd]->get_fs_file()->get_index_block()*blockSize+blockSize; j++)
                {
                    char *tmp;
                    char ch='\0';
                    // check we are writing to correct location
                    fseek(sim_disk_fd, j, SEEK_SET);
                    fread(&ch, 1, 1, sim_disk_fd);
                    fseek(sim_disk_fd, j, SEEK_SET);
                    if(ch=='\0')
                    {
                        // convert int to char
                        *tmp = ((index-1)/blockSize)+48;
                        fwrite (tmp , sizeof(char), sizeof(char), sim_disk_fd);
                        break;
                    }
                }
                    
                // update file size
                if(len>blockSize)
                    OpenFileDescriptors[fd]->get_fs_file()->set_file_size(OpenFileDescriptors[fd]->get_fs_file()->get_file_size()+blockSize);
                else
                    OpenFileDescriptors[fd]->get_fs_file()->set_file_size(OpenFileDescriptors[fd]->get_fs_file()->get_file_size()+len);                
                    
            }
                
                
            else
            {
                // write to previous block ....
                count++;
                    
                // find last index block
                for(int i = OpenFileDescriptors[fd]->get_fs_file()->get_index_block()*blockSize+blockSize-1;
                        i >= OpenFileDescriptors[fd]->get_fs_file()->get_index_block()*blockSize; i--)
                {
                        
                    char ch='\0';
                    fseek(sim_disk_fd, i, SEEK_SET);
                    fread(&ch, 1, 1, sim_disk_fd);
                        
                    // found it !
                    if(ch!='\0')
                    {
                        block = ch-48;
                        fseek(sim_disk_fd, block*blockSize, SEEK_SET);
                        
                        for(int j=0; j<blockSize; j++)
                        {
                            // go to where last index tells us
                            fseek(sim_disk_fd, block*blockSize+j, SEEK_SET);
                            fread(&ch, 1, 1, sim_disk_fd);
                            if(ch=='\0')
                            {
                                // write to disc
                                fseek(sim_disk_fd, block*blockSize+j, SEEK_SET);
                                for( ; check <count*blockSize-j; check++)
                                {
                                    if(check<len)
                                    {
                                        fwrite (buf+check , sizeof(char), sizeof(char), sim_disk_fd);
                                    }
                                    
                                }
                            }
                        }
                        // update buf to represent the chars we did NOT write to disc
                        if(len>blockSize- OpenFileDescriptors[fd]->get_fs_file()->get_file_size()%blockSize)
                        {
                            strncpy(buf,&buf[check],len-check);
                            buf[len-check]='\0';                        
                        }
                            
                        break;
                    }
                }
                    
                // update file size
                if(len>blockSize- OpenFileDescriptors[fd]->get_fs_file()->get_file_size()%blockSize)
                    OpenFileDescriptors[fd]->get_fs_file()->set_file_size(OpenFileDescriptors[fd]->get_fs_file()->get_file_size()+blockSize- OpenFileDescriptors[fd]->get_fs_file()->get_file_size()%blockSize);
                else
                    OpenFileDescriptors[fd]->get_fs_file()->set_file_size(OpenFileDescriptors[fd]->get_fs_file()->get_file_size()+len);
            }
                
        }
        
    return 0;    
    }
    
    // ------------------------------------------------------------------------
    
    int DelFile( string FileName ) 
    {
        // go thorugh MainDir
        for (int i = 0; i < MainDir.size(); i++)
        {
            // find our file
            if(MainDir[i]!=NULL && MainDir[i]->getFileName()==FileName)
            {
                // check index block
                for(int j = MainDir[i]->get_fs_file()->get_index_block()*blockSize; j<MainDir[i]->get_fs_file()->get_index_block()*blockSize+blockSize; j++)
                {
                    char ch='\0';
                    fseek(sim_disk_fd, j, SEEK_SET);
                    fread(&ch, 1, 1, sim_disk_fd);
                    if(ch!='\0')
                    {
                        // cast index to int
                        int n = ch-48;
                        // delete file content
                        for(int k=n*blockSize; k<n*blockSize+blockSize; k++)
                        {
                            fseek(sim_disk_fd, k, SEEK_SET);
                            fwrite("\0", 1, 1, sim_disk_fd);
                            BitVector[k/blockSize]=0;
                        }
                        // delete index block
                        fseek(sim_disk_fd, j, SEEK_SET);
                        fwrite("\0", 1, 1, sim_disk_fd);
                    }
                    BitVector[j/blockSize]=0;
                }
                
                // update OpenFileDescriptors
                for (int j = 0; j < OpenFileDescriptors.size(); j++)
                {
                    if(OpenFileDescriptors[j]!=NULL && MainDir[i]->getFileName()==OpenFileDescriptors[j]->getFileName())
                    {
                        OpenFileDescriptors[j]=NULL;
                    }
                }
                delete(MainDir[i]->get_fs_file());
                delete(MainDir[i]);
                MainDir[i]=NULL;
                return i;
            }
        }
    return -1;
    }
    
    // ------------------------------------------------------------------------

    // Read from file of given fd into buf , returns 0 on success -1 when failed
    int ReadFromFile(int fd, char *buf, int len ) 
    {
        strcpy(buf, "\0");
        
        // disc was NOT formatted
        if(is_formated == false)
            return -1;
        
        // fd is too big or too small
        if(fd > OpenFileDescriptors.size() || fd < 0)
            return -1;
            
        // no such file
        if(OpenFileDescriptors[fd]==NULL)
            return -1;
        
        // file is close
        if(MainDir[fd]->get_in_use()==false)
            return -1;
        
        // length of string is too big for our file
        if(len > blockSize*blockSize)
            return -1;
        
        if(OpenFileDescriptors[fd]->get_fs_file()->get_file_size() < len)
            return -1;
        
        
        // count how many chars we have already read to disc    
        int count = 0;
        while(count<len)
        {
            // go through index block
            for(int i = OpenFileDescriptors[fd]->get_fs_file()->get_index_block()*blockSize;
                i < OpenFileDescriptors[fd]->get_fs_file()->get_index_block()*blockSize+blockSize; i++ )
            {
                // find block to read from
                char ch='\0';
                int num = 0;
                fseek(sim_disk_fd, i, SEEK_SET);
                fread(&ch, 1, 1, sim_disk_fd);
                if(ch!='\0')
                {
                    num = ch-48;
                    // go through block
                    for(int j=0; j<blockSize; j++)
                    {
                        // read 
                        fseek(sim_disk_fd, num*blockSize+j, SEEK_SET);
                        fread(&ch, 1, 1, sim_disk_fd);
                        if(ch!='\0')
                        {
                            // just in case
                            if(count==len)
                                break;
                            count++;
                            // add char to our string
                            strncat(buf, &ch, 1);
                        }
                    }
                }
            }
        }
        return 0;
    }
};
    
    // ------------------------------------------------------------------------

    
void show_menu()
{
    
    printf("0 : End program\n1 : Print open file descriptors and disc content\n");
    printf("2 : Format disc to specific block size provided by the user\n");
    printf("3 : Create new file\n4 : Open file\n5 : Close file\n6 : Write to file\n");
    printf("7 : Read from file\n8 : Delete file\n");
}


    // ------------------------------------------------------------------------
    //                       _ __ ___   __ _ _ _ ___  
    //                      | '_ ` _ \ / _` | | '_  \ 
    //                      | | | | | | (_| | | | | |
    //                      |_| |_| |_|\__,_|_|_| |_|
    //  
    // ------------------------------------------------------------------------


    
    
int main() 
{
    int blockSize; 
	int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read; 
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while(1) 
    {
        show_menu();
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
				delete(fs);
				exit(0);
                break;

            case 1:  // list-file
                fs->listAll(); 
                break;
          
            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;
          
            case 3:    // create-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            
            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
             
            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd); 
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
           
            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );
                break;
          
            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile: " << str_to_read << endl;
                break;
           
            case 8:   // delete file 
                 cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }
} 
