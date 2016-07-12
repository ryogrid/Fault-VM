#! /usr/bin/ruby
require 'net/http'
require 'thread'

READ_TIMEOUT_SEC = 2
OPEN_TIMEOUT_SEC = 2 #connetion timout
REQUEST_HOST="192.168.1.1"
REQUEST_PATH="/cgi-bin/heavy.cgi"

def request_a_page(host,path)
  Net::HTTP.version_1_2
  req = Net::HTTP::Get.new(path)
  
  http = Net::HTTP.new(host)
  http.read_timeout=READ_TIMEOUT_SEC
  http.open_timeout=OPEN_TIMEOUT_SEC

  return http.request(req).body
end

def serialize_doc_to_file()
  doc = request_a_page(REQUEST_HOST,REQUEST_PATH)
  open("./lvs_document.html","w"){ |file|
    file.write(doc)
  }
end

if ARGV.length == 1 && ARGV[0] == "create_data"
  puts "create serialized document data"
  serialize_doc_to_file()
  exit()
elsif ARGV.length == 1 && ARGV[0]
  puts "usage: request_analyzer.rb create_data"
  exit()
end

#request_count=ARGV[0].to_i
file = open("./request.log","w")

#constant
THREAD_COUNT_LIMIT=10
SLEEP_SEC_TO_COMPLETE_REQUEST=0.0001
LOG_PERIOD_SEC=1.0

#mutex
m = Mutex.new

#read serialized data
having_doc_data = nil
open("./lvs_document.html"){ |doc_file|
  having_doc_data = doc_file.read()
}

#init each count
current_div_count=0
connection_error_count=0
current_thread_count=0
bad_doc_count=0
time_count=1

#output start time to file
start_time = Time.now
start_time_f = start_time.to_f
file.puts(start_time.to_s)

#init time
before_time = start_time.to_f

#output format comment
file.puts("#elapsed sec , requests per predetermined sec , connection error , bad documents")

#request predetermined times with thrughput per 1 sec
#continue loop forever
while true
  #check which predetermined time elapsed
  now_f = Time.now.to_f
  if((now_f - start_time_f) > time_count)
    m.synchronize{
      file.puts((now_f - start_time_f).to_s + "," + current_div_count.to_s + "," + connection_error_count.to_s  + "," + bad_doc_count.to_s)
      file.flush()
      current_div_count = 0
      connection_error_count = 0
      bad_doc_count = 0
    }
    before_time = now_f
    time_count += LOG_PERIOD_SEC
  end

  if(current_thread_count < THREAD_COUNT_LIMIT)
    #create thread to request once
    t = Thread.new do
      begin
        m.synchronize{
          current_thread_count+=1
        }
        doc = request_a_page(REQUEST_HOST,REQUEST_PATH)
        #check if request got is same to having data
        if(doc != having_doc_data)
          m.synchronize{
            bad_doc_count +=1
          }
        end
        m.synchronize{
          current_div_count+=1
        }
      rescue => e
        m.synchronize{
          connection_error_count+=1
        }
        p e
      rescue Timeout::Error
        m.synchronize{
          connection_error_count+=1
        }
      ensure
        m.synchronize{
          current_thread_count-=1
        }
      end
    end
  else
    sleep(SLEEP_SEC_TO_COMPLETE_REQUEST)
  end
end
