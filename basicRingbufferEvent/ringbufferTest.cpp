  
/*
 * Author: Harry van Haaren <harryhaaren@gmail.com> 2013
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

// compile with: g++  -g -o ringbuffer ringbufferTest.cpp `pkg-config --cflags --libs jack`

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <jack/jack.h>
#include <jack/ringbuffer.h>

using namespace std;

// a global pointer to a ring buffer (usually a class will own this pointer)
char* mem = 0;
jack_ringbuffer_t *buffer = 0;


enum {
  EVENT_ONE = 0,
  EVENT_TWO = 1,
  EVENT_ACK = 2,
};

class EventBase
{
  public:
    
    virtual ~EventBase() {}
    
    virtual int type() = 0;
    virtual uint32_t size() = 0;
};

class EventOne : public EventBase
{
  public:
    int type() { return int(EVENT_ONE); }
    uint32_t size() { return sizeof(EventOne); }
    
    void* voidPtr;
    int trackNum;
    int slotNum;
    
    EventOne()
    {
      voidPtr = 0;
      trackNum = 0;
      slotNum = 2;
    }
};

class EventTwo : public EventBase
{
  public:
    int type() { return int(EVENT_TWO); }
    uint32_t size() { return sizeof(EventTwo); }
    
    EventTwo()
    {
    }
};

// ACK data event
class EventAck : public EventBase
{
  public:
    int type() { return int(EVENT_ACK); }
    uint32_t size() { return sizeof(EventAck); }
    
    int waste;
    
    EventAck()
    {
    }
};


int write(int i)
{
  EventAck e;
  EventOne o;
  EventTwo t;
  
  if ( i % 100 == 0 && i > 0 )
  {
    jack_ringbuffer_write( buffer, (const char*)&o, o.size() );
  }
  else if ( i % 5 == 0 )
  {
    jack_ringbuffer_write( buffer, (const char*)&t, t.size() );
  }
  else
  {
    jack_ringbuffer_write( buffer, (const char*)&e, e.size() );
  }
}

int process(jack_nframes_t nframes, void* )
{
  // check if there's anything to read
  int availableRead = jack_ringbuffer_read_space(buffer);
  
  if ( availableRead >= sizeof(EventBase) )
  {
    jack_ringbuffer_peek( buffer, mem, sizeof(EventBase) );
    
    EventBase* e = (EventBase*)mem;
    
    //cout << "JACK:: read event  " << e->type() << endl;
    
    switch ( e->type() )
    {
      case EVENT_ACK:
      {
        EventAck ev;
        jack_ringbuffer_read( buffer, (char*)&ev, sizeof(EventAck) );
        cout << "Event: Ack" << endl; // non RT but shows action took place
        break;
      }
      case EVENT_ONE:
      {
        EventOne ev;
        jack_ringbuffer_read( buffer, (char*)&ev, sizeof(EventOne) );
        cout << "\t\tEvent: One" << endl; // non RT but shows action took place
        break;
      }
      case EVENT_TWO:
      {
        EventTwo ev;
        jack_ringbuffer_read( buffer, (char*)&ev, sizeof(EventTwo) );
        cout << "\t\t\t\tEvent: Two" << endl; // non RT but shows action took place
        break;
      }
      
    }
  }
  
  return 0;
}


int main()
{
  std::cout << "Ring buffer tutorial" << std::endl;
  
  // create an instance of a ringbuffer that will hold up to 20 integers,
  // let the pointer point to it
  buffer = jack_ringbuffer_create( 200 * sizeof(EventBase));
  
  // lock the buffer into memory, this is *NOT* realtime safe, do it before
  // using the buffer!
  int res = jack_ringbuffer_mlock(buffer);
  
  // check if we've locked the memory successfully
  if ( res ) {
    std::cout << "Error locking memory!" << std::endl;
    return -1;
  }
  
  // create memory to "peek" at an event from ringbuffer
  mem = (char*)malloc( sizeof(EventBase) );
  
  // create a JACK client, register the process callback and activate
  jack_client_t* client = jack_client_open ( "RingbufferDemo", JackNullOption , 0 , 0 );
  jack_set_process_callback  (client, process , 0);
  jack_activate(client);
  
  for ( int i = 1; i < 9000; i++)
  {
    // write an event, then pause a while, JACK will get a go and then
    // we'll write another event... etc
    write(i);
    usleep(20000);
  }
  
  jack_deactivate( client );
  
  free( mem );
  
  return 0;
}
