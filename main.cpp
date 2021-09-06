#include "mbed.h"
#include "SDIOBlockDevice.h"
#include "FATFileSystem.h"
#include "AudioPlayer.h"
#include <vector>
#include <string>


DigitalOut led1(LED1);

// File system on SD card connected over SDIO
#define MOUNT_PATH "sd"
static SDIOBlockDevice sd;
BlockDevice *BlockDevice::get_default_instance() { return &sd; }
BlockDevice *get_other_blockdevice() { return &sd; }
BlockDevice *blockDevice = BlockDevice::get_default_instance();
FATFileSystem fs(MOUNT_PATH, blockDevice);

PwmOut pwm_out(PA_0);
AudioPlayer player(&pwm_out);
Thread audio_thread;

int n = -1;
InterruptIn sw(BUTTON1);
std::vector<string> files;

// change the index of *.wav file
void buttop_handler()
{
    // get index of next file
    n++;
    if( n >= files.size() )    
        n=0;
    // stop playing
    player.stop();    
}

// Get all BMP on the root dir of "fs"
std::vector<std::string> get_files_by_ext(const char *ext){  
  std::vector<std::string> files;  
  std::string dir_path = std::string("/") + std::string(MOUNT_PATH) + std::string("/");
  DIR *d = opendir( dir_path.c_str() );
  if (!d) {
    error("error: %s (%d)\n", strerror(errno), -errno);
  }
  else{
    while (true) {
        struct dirent *e = readdir(d);
        if (!e) {
        break;
        }
        if( strstr(e->d_name, ext) != NULL ){
            files.push_back(std::string(e->d_name));
            printf("%s\n", e->d_name);
        }
    }
    int err = closedir(d);
    if (err < 0) {
        error("error: %s (%d)\n", strerror(errno), -errno);
  }

  }
  return files;
}


void dump_data( std::string &file_name ){
#ifdef DUMP_DATA	    
    std::string dump_file_name = file_name; 
    size_t  index = dump_file_name.find(".wav", 0);
    dump_file_name.replace(index, 4, ".dat");
    printf( "dump 2 seconds of  [n=%d] %s to file %s\n", n, file_name.c_str(),  dump_file_name.c_str() );
    File dump_file;
    int err = dump_file.open( &fs, dump_file_name.c_str(), O_CREAT|O_WRONLY|O_BINARY ); 
    if( err != 0)
    { 
        printf( "can't open file for writing %s, err = %d %s\n", dump_file_name.c_str(), err, strerror(-err));
        return;
    }  


    for( size_t i=0; i<player._data_max; i++ )
        dump_file.write( player._data+i, sizeof(float) );

    err = dump_file.sync();
    if( err != 0)
    { 
        printf( "can't sync file %s, err = %d %s\n", dump_file_name.c_str(), err, strerror(-err));        
    }     
    dump_file.close();    
    printf( "finish dump\n");
#endif
    return;
}

void play_audio() {
    pwm_out.period_us(31);        
    //speaker.period(1.0/320000.0);
    sw.rise(buttop_handler);

    sd.init();
    fs.mount(&sd);
    files = get_files_by_ext(".wav");

    if( files.size() < 0 ){
        printf("No *.wav files on SD card\n");
    }
    else {
        n = 0;
        FILE *wav_file;
        while (1) {
    //            char *name = "/sd/NP_4100-6200_16fps.wav";
            int l_n = n;
            File wav_file( &fs, files[n].c_str() );
            //auto &file = files[n];
            printf( "play [n=%d] %s\n", l_n, files[l_n].c_str() );            
            //wav_file = fopen( file.c_str(), "r" );
            player.play( &wav_file );
            wav_file.close();
            dump_data( files[l_n] );
            //buttop_handler();
            //fclose( wav_file );
        }
    }
    fs.unmount();
    sd.deinit();    
}


// main() runs in its own thread in the OS
int main()
{
    audio_thread.start(play_audio);
    while (true) {
        ThisThread::sleep_for(500ms);
        led1 = !led1;        
    }
}

