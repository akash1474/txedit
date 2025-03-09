#!/usr/bin/env python
"""Test file for Python syntax highlighting in editors / IDEs.

Meant to cover a wide range of different types of statements and expressions.
Not necessarily sensical or comprehensive (assume that if one exception is
highlighted that all are, for instance).

Extraneous trailing whitespace can't be tested because of svn pre-commit hook
checks for such things.

"""
# Comment
# OPTIONAL: XXX catch your attention
# TODO(me): next big thing
# FIXME: this does not work

# Statements
from __future__ import with_statement  # Import
from sys import path as thing
from sys.name import something
import username.name

print(thing)

assert True  # keyword


def foo():  # function definition
    return []


class Bar(object):  # Class definition
	name="akash"
	age=20
    def __enter__(self):
        pass
	
	def __init__(self,name,age=30):
		self.name=name
		self.age=age
	
	def do_something():
		pass

    def __exit__(self, *args):
        pass

user=Bar()
user.do_something()


foo()  # UNCOLOURED: function call
while False:  # 'while'
    continue
for x in foo():  # 'for'
    break
with Bar() as stuff:
    pass
if False:
    pass  # 'if'
elif False:
    pass
else:
    pass

# Constants
'single-quote', u'unicode'  # Strings of all kinds; prefixes not highlighted
"double-quote"
"""triple double-quote"""
'''triple single-quote'''
r'raw'
ur'unicode raw'
'escape\n'
'\04'  # octal
'\xFF'  # hex
'\u1111'  # unicode character
1  # Integral
1L
1.0  # Float
.1
1+2j  # Complex

# Expressions
1 and 2 or 3  # Boolean operators
2 < 3  # UNCOLOURED: comparison operators
spam = 42  # UNCOLOURED: assignment
2 + 3  # UNCOLOURED: number operators
[]  # UNCOLOURED: list
{}  # UNCOLOURED: dict
(1,)  # UNCOLOURED: tuple
all  # Built-in functions
GeneratorExit  # Exceptions


import base64
import os
import argparse
import math
from getpass import getpass
from colorit import *
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.fernet import Fernet

def Crypto():
    password=getpass(prompt="Key: ")
    salt=b"\xae\xd5\xcfj\xd1\xad\ry'\x0fm\xd6>\xf4\xad\x9d"
    kdf=PBKDF2HMAC(
        algorithm=hashes.SHA512(),
        length=32,# affects the size
        salt=salt,
        iterations=37689,
        backend=default_backend()
        )
    gen_key = base64.urlsafe_b64encode(kdf.derive(password.encode()))
    return Fernet(key=gen_key)

init_colorit()

def ui_update(string):
    sys.stdout.write(f'{string}\r')
    sys.stdout.flush()

def print_msg_box(msg, indent=1, width=None, title=None):
    """Print message-box with optional title."""
    lines = msg.split('\n')
    space = " " * indent
    if not width:
        width = max(map(len, lines))
    box = f'╔{"═" * (width + indent * 2)}╗\n'.center(os.get_terminal_size()[0])  # upper_border
    if title:
        box += f'║{space}{title:<{width}}{space}║\n'  # title
        box += f'║{space}{"-" * len(title):<{width}}{space}║\n'  # underscore
    box += ''.join([f'║{space}{color(line,Colors.orange):<{width}}{space}║\n' for line in lines])
    box += f'╚{"═" * (width + indent * 2)}╝'.center(os.get_terminal_size()[0])  # lower_border
    print(box)

def draw_line(char="-",center=False):
    if(center and len(char)==1):
        count=int(os.get_terminal_size()[0]//2)
        print((char*count).center(os.get_terminal_size()[0]))
    elif(len(char)==1):
        print(char*os.get_terminal_size()[0])
    else:
        raise f"{char} should be of length 1"


def show_doc():
    print_msg_box("crypto v0.1.2")
    # print(color("crypto v0.1.1".center(os.get_terminal_size()[0]),Colors.orange))
    print(color("Documentation:",Colors.green))
    print("   »",color("--encrypt/-e:",Colors.blue),"Encrypting the file")
    print("   »",color("--decrypt/-d:",Colors.blue),"Decrypting the file")
    print("   »",color("--all:",Colors.blue),"Selecting all files in current dir")
    print("   »",color("--file/-f:",Colors.blue),"To provide a file")
    print("   »",color("--files/-fls:",Colors.blue),"To select multiple files")
    print("   »",color("--doc/-i:",Colors.blue),"Show Documentation\n")
    print(color(" ☼",Colors.red),color("Encrypting and Decrypting a file",Colors.yellow))
    print("   Encrypting »",color("crypto -e -f file.ext",Colors.orange))
    print("   Decrypting »",color("crypto -d -f file.ext\n",Colors.orange))
    print(color("\n ☼",Colors.red),color("Encrypting all the file in a folder",Colors.yellow))
    print("   Encrypting »",color("crypto -e --all",Colors.orange))
    print("   Decrypting »",color("crypto -d --all",Colors.orange))
    print(color("\n ☼",Colors.red),color("Encrypting and Decrypting multiple specific file",Colors.yellow))
    print("   Encrypting »",color("crypto -e -fls file1.ext file2.ext filen.ext",Colors.orange))
    print("   Decrypting »",color("crypto -d -fls file1.ext file2.ext filen.ext\n",Colors.orange))

def convert_size(size_bytes):
    if size_bytes == 0:
        return "0B"
    size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
    i = int(math.floor(math.log(size_bytes, 1024)))
    p = math.pow(1024, i)
    s = round(size_bytes / p, 2)
    return "%s %s" % (s, size_name[i])

def show_file_stats(file,s1,s2,enc=False):
    print(file,color(convert_size(s1),Colors.orange),color(" » ",Colors.green),color(convert_size(s2),Colors.red))
    print(color(f"{'Encryption' if enc else 'Decryption'} Completed",Colors.purple))

## Body ##
def encrypt(filename,fernet,show_stats=False):
    s1=os.path.getsize(filename)
    s2=0
    with open(filename,'r+b') as f:
        data=f.read()
        enc=fernet.encrypt(data)
        with open(filename,'wb') as enc_file:
            enc_file.write(enc)
            enc_file.close()
        f.close()
        s2=os.path.getsize(filename)

    if(show_stats):
        show_file_stats(filename,s1,s2,enc=True)

    return s2-s1

def decrypt(filename,fernet,show_stats=False):
    s1=os.path.getsize(filename)
    s2=0
    with open(filename,'r+b') as f:
        data=f.read()
        dec=fernet.decrypt(data)
        with open(filename,'wb') as dec_file:
            dec_file.write(dec)
            dec_file.close()
        f.close()
        s2=os.path.getsize(filename)

    if(show_stats):
        show_file_stats(filename,s1,s2)
    return s1-s2

def enc_all(arr=os.listdir()):
    fernet=Crypto()
    print(color("Encrypting Files....",Colors.purple))
    count=0 
    for file in arr:
        try:
            ds=encrypt(file,fernet)
            count+=1
            pr_str="{0} {1:<38} {2}".format(color("Encrypted:  ",Colors.green),color(f"δs={convert_size(ds)}",Colors.orange),file)
            print(pr_str)
            ui_update(color(count,Colors.blue)+" files were encrypted")
        except:
            print(color("Failed:  ",Colors.red),file)
    print(color("Encryption Process Completed",Colors.purple))
    print("---------------------")
    print(color(count,Colors.blue)," files were encrypted")

def dec_all(arr=os.listdir()):
    fernet=Crypto()
    print(color("Decrypting Files....",Colors.purple))
    count=0
    for file in arr:
        try:
            ds=decrypt(file,fernet)
            count+=1
            pr_str="{0} {1:<38} {2}".format(color("Decrypted:  ",Colors.green),color(f"δs=-{convert_size(ds)}",Colors.orange),file)
            print(pr_str)
            ui_update(color(count,Colors.blue)+" files were encrypted")
        except:
            print(color("Failed:  ",Colors.red),file)
    print(color("Decryption Process Completed",Colors.purple))
    print("---------------------")
    print(color(count,Colors.blue)," files were decrypted")



parser = argparse.ArgumentParser()
## arguments ##
parser.add_argument('--encrypt', '-e',action="store_true", dest='encrypt',help="encrypt file")
parser.add_argument('--file','-f',dest="file",help="file name")
parser.add_argument('--decrypt','-d',action="store_true", dest='decrypt',help="logging out of the google drive account")
parser.add_argument('--all',action="store_true", dest='allFiles',help="encrypt all the files")
parser.add_argument('--files','-fls',nargs='*', action='append',dest='files',help="name of file in as list:string")
parser.add_argument('--doc', '-i',action="store_true", dest='document',help="show documentation")
## argument ##
args=parser.parse_args()

if(args.document):
    show_doc()
elif(not args.allFiles and not args.files and args.encrypt and args.file):
    try:
        fernet=Crypto()
        encrypt(args.file,fernet,show_stats=True)
    except:
        print(color("UNKNOW ERROR:",Colors.orange))
        print(color("   » Encryption Failed",Colors.red),args.file)
elif(not args.allFiles and not args.files and args.decrypt and args.file):
    try:
        fernet=Crypto()
        decrypt(args.file,fernet,show_stats=True)
    except:
        print(color("Invalid Key Error:",Colors.orange))
        print("  » ",color("Decryption Failed",Colors.red),args.file)
elif(not args.files and not args.file and args.allFiles and args.encrypt):
    enc_all()
elif(not args.files and not args.file and args.allFiles and args.decrypt):
    dec_all()
elif(not args.allFiles and args.encrypt and args.files):
    enc_all(arr=args.files[0])
elif(not args.allFiles and args.decrypt and args.files):
    dec_all(arr=args.files[0])
else:
    show_doc()





import pickle
import os
import sys
from colorit import *
from google_auth_oauthlib.flow import Flow, InstalledAppFlow
from googleapiclient.discovery import build
from google.auth.transport.requests import Request
from google.api_core.exceptions import NotFound
import temp


init_colorit()


def temp_dir():
    tempa = temp.tempdir().split(os.sep)
    tempa.pop()
    temp_dir = "/".join(tempa)
    return temp_dir

def application_path():
   if getattr(sys, 'frozen', False):
      return os.path.dirname(sys.executable)
   elif __file__:
      return os.path.dirname(__file__)


def login_fail():
   print("Tips:")
   print(color("    1.Please run CMD using administrator",Colors.orange))
   print(color("    2.Running as CMD as administrator is required when application runs for the first time",Colors.orange))

def create_service(client_secret_file, api_name, api_version, *scopes):
    # print(client_secret_file, api_name, api_version, scopes, sep='-')
    CLIENT_SECRET_FILE = client_secret_file
    API_SERVICE_NAME = api_name
    API_VERSION = api_version
    SCOPES = [scope for scope in scopes[0]]
    cred = None

    pickle_file = f'token_{API_SERVICE_NAME}_{API_VERSION}.pickle'
    pickle_file_location=os.path.join(temp_dir(),pickle_file)
    if os.path.exists(pickle_file_location):
        with open(pickle_file_location, 'rb') as token:
            cred = pickle.load(token)

    if not cred or not cred.valid:
        if cred and cred.expired and cred.refresh_token:
            cred.refresh(Request())
        else:
            print(color("Logging In",Colors.orange))
            flow = InstalledAppFlow.from_client_secrets_file(CLIENT_SECRET_FILE, SCOPES)
            cred = flow.run_local_server()

        with open(pickle_file_location, 'wb') as token:
            pickle.dump(cred, token)

    try:
        service = build(API_SERVICE_NAME, API_VERSION, credentials=cred)
        return service
    except ConnectionError:
        print(color("!!!  No Internet ConnectionError  !!!",Colors.orange))
        print(color("!!!  Unable to connect to server  !!!",Colors.red))
        exit()
    except PermissionError:
        login_fail()
        exit()
    except Exception as e:
        print(color("!!!  ERROR  !!!",Colors.red))
        print(e)
        print(color("!!!  ERROR  !!!",Colors.red))
        exit()

def convert_to_RFC_datetime(year=1900, month=1, day=1, hour=0, minute=0):
    dt = datetime.datetime(year, month, day, hour, minute, 0).isoformat() + 'Z'
    return dt



#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
#######################################################################################################
#################################### GOOGLE DRIVE FILE_MANAGER ########################################
#######################################################################################################
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
###
### Function: Allows you view your google drive files and manage them
###
### Purpose:  Recently,The use of google drive has increased incredibly and to reduce that pain to open
###           to opening google drive in your browser I created created this simple and easy to handle
###           google drive manager.
###           
###           This will help you to DOWNLOAD UPLOAD DELETE CREATE files in google drive thus easing your 
###           pain of having to open browser and then navigating to google drive.
###
###           Google offer its drive functionality to manage your files and stuff.This program uses 
###           that API to connect to your google drive and fetch all your files.
###
### NOTES:    1. The script also you to explorer files with in-built file explorer
###
###           2. To use this script, you will need to obtain your own API credentials file by making
###                   a project via the Google Developers Console (aka 'Google Cloud Platform').
###              The credential file should be re-named 'client_secret.json' and be placed in the 
###              same directory as this script.
###                        >>> See the Readme for instructions on this.
###
###           3. I suck at programming so if something doesn't work I'll try to fix it but might not
###              even know how, so don't expect too much.
###
###
### Author:   Akash Pandit - github.com/akash1474
###                        - instagram.com/panditakash38
###
### IMPORTANT:  I OFFER NO WARRANTY OR GUARANTEE FOR THIS SCRIPT. USE AT YOUR OWN RISK.
###             I tested it on my own and implemented some failsafes as best as I could,
###             but there could always be some kind of bug. You should inspect the code yourself.
version = "2.2.5"
configVersion = 2
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#




# Standard Modules
import io
import os
import math
from pathlib import Path
import json
import sys
from time import time



# Non Standard Modules
from colorit import *
from PyInquirer import prompt, style_from_dict, Token
from yaspin import yaspin
import temp



# Google Authentication Modules
from GoogleLogin import application_path
from googleapiclient.http import MediaFileUpload, MediaIoBaseDownload
from GoogleLogin import create_service

init_colorit()
custom_style = style_from_dict({
    Token.Separator: '#cc5454',
    Token.QuestionMark: '#673ab7 bold',
    Token.Selected: '#cc5454',  # default
    Token.Pointer: '#673ab7 bold',
    Token.Answer: '#f44336 bold',
})

DOWNLOAD_LOCATION = str(os.path.join(Path.home(), "Downloads"))
SCOPES = ['https://www.googleapis.com/auth/drive.file','https://www.googleapis.com/auth/drive.metadata', 'https://www.googleapis.com/auth/drive.appdata']
GOOGLE_DRIVE = create_service(os.path.join("./",'credentials.json'), "drive","v3", ['https://www.googleapis.com/auth/drive'])


def ask(query):
   if(input(query)=="y"):
      return True
   else:
      return False

def temp_dir():
    tempa = temp.tempdir().split(os.sep)
    tempa.pop()
    temp_dir = "/".join(tempa)
    return temp_dir


def print_msg_box(msg, indent=1, width=None, title=None):
    """Print message-box with optional title."""
    lines = msg.split('\n')
    space = " " * indent
    if not width:
        width = max(map(len, lines))
    box = f'╔{"═" * (width + indent * 2)}╗\n'.center(os.get_terminal_size()[0])  # upper_border
    if title:
        box += f'║{space}{title:<{width}}{space}║\n'  # title
        box += f'║{space}{"-" * len(title):<{width}}{space}║\n'  # underscore
    box += ''.join([f'║{space}{color(line,Colors.orange):<{width}}{space}║\n' for line in lines])
    box += f'╚{"═" * (width + indent * 2)}╝'.center(os.get_terminal_size()[0])  # lower_border
    print(box)

def draw_line(char="-",center=False):
   if(center and len(char)==1):
      count=int(os.get_terminal_size()[0]//2)
      print((char*count).center(os.get_terminal_size()[0]))
   elif(len(char)==1):
      print(char*os.get_terminal_size()[0])
   else:
      raise f"{char} should be of length 1"

def ui_update(string):
    sys.stdout.write(f'{string}\r')
    sys.stdout.flush()

if(len(sys.argv)==2 and (sys.argv[1]=="-lo" or sys.argv[1]=="--logout")):
   token_file=os.path.join(temp_dir(),"token_drive_v3.pickle")
   if(not os.path.exists(token_file)):
      print(color("!!! You are not logged in !!!",Colors.red))
      exit()




def logout():
   token_file=os.path.join(temp_dir(),"token_drive_v3.pickle")
   if(os.path.exists(token_file)):
      os.remove(token_file)
      print(color("!!!  Logged Out Successfully  !!!",Colors.orange))
   else:
      print(color("!!! You are not logged in !!!",Colors.red))


def login():
   print(color("!!! __Logged In Successfully__  !!!",Colors.green))


def convert_size(size_bytes):
    """
    Convert Bytes to size
    """
    if size_bytes == 0:
        return "0B"
    size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
    i = int(math.floor(math.log(size_bytes, 1024)))
    p = math.pow(1024, i)
    s = round(size_bytes / p, 2)
    return "%s %s" % (s, size_name[i])

def calc_speed(size):
   if(size>=1048576):
      return f"{round(size/1048576,2)} MB/s"
   else:
      return f"{round(size/1024)} KB/s"

def search_file(query):
    return GOOGLE_DRIVE.files().list(
       q=query, fields="files(name,id,mimeType,kind,size)").execute()
    

def file_info(id):
    """Fild Info
    @param id:String
    returns {
         id
         name
         mimeType,
         kind,
         size
    }
    """
    return GOOGLE_DRIVE.files().get(fileId=id, fields="id,name,size,kind,mimeType",supportsAllDrives=True).execute()
    


def show_info(id):
   """Print the file Info
   @param id:String
   "--------------------------------------------------------"
   "FileName: ",fileStats['name']
   "FileSize: ",convert_size(int(fileStats['size']))
   "--------------------------------------------------------"
   """
   fileStats=file_info(id)
   print("--------------------------------------------------------")
   for key in fileStats:
      if(key=="size"):
         print("{0:<32}: {1}".format(color(key,Colors.blue),color(convert_size(int(fileStats['size'])),Colors.orange)))
         break
      print("{0:<32}: {1}".format(color(key,Colors.blue),color(fileStats[key],Colors.orange)))
   print("--------------------------------------------------------")


def get_folders_list():
   """List of folder in your google drive"""
   return search_file("mimeType='application/vnd.google-apps.folder'")['files']


def show_folders_info():
   """Info of folder
   @param folders:Object
   """
   folders=get_folders_list()
   for el in folders:
      print(el['name'])


def create_folder(name):
   """Create Folder
   @param folder_name:String
   """
   file_metadata = {
    'name': name,
    'mimeType': 'application/vnd.google-apps.folder'
   }
   data= GOOGLE_DRIVE.files().create(body=file_metadata,fields='id,name,mimeType').execute()
   print("Folder Created ",data['id'])





#######################################################################################################
####################################       DOWNLOADING         ########################################
#######################################################################################################



def download_file(id,location,prompt=True,from_folder=False):
   """Download File
   @params
      id:file_Id
      location:download_location
      prompt:boolean
      from_folder:boolean
   """
   fileStats=file_info(id)
   if(from_folder):
      # print(color(convert_size(fileStats['size']),Colors.orange),color(fileStats['name'],Colors.green))
      download(id,fileStats,location)
   else:
      print("--------------------------------------------------------")
      print("FileName: ",fileStats['name'])
      print("FileSize: ",convert_size(int(fileStats['size'])))
      print("--------------------------------------------------------")
      if prompt:
         reply=input("Are You Sure You want to download?(y/n): ")
      else:
         reply="y"

      if(reply=="y"):
         download(id,fileStats,location)
         print("\n============ DOWNLOAD COMPLETED ==============")
   



def download(id,fileStats,location="./"):
   try:
      if os.path.exists(os.path.join(location, fileStats['name'])):
         print('{1:<35}  {2:<32}\t{0}'.format(fileStats['name'],color(convert_size(int(fileStats['size'])),Colors.orange),color("EXISTS",Colors.green)))
         return None

      request = GOOGLE_DRIVE.files().get_media(fileId=id)
      fh = io.FileIO(os.path.join(location,fileStats['name']+".download"),"wb")
      downloader = MediaIoBaseDownload(fh, request,chunksize=1048576)
      done = False
      start = time()
      while done is False:
         status, done = downloader.next_chunk()
         dt=time() - start
         if(dt==0.0):
            dt=0.1

         # props
         file_name=fileStats['name']
         file_size=color(convert_size(int(fileStats['size'])),Colors.orange)
         percentage=color(f'{"DONE" if round(status.progress()*100,2)==100.0 else str(round(status.progress()*100,2))+"%"}',Colors.green)
         download_speed=color(calc_speed((downloader._progress)/dt),Colors.red)
         progress_bar=""
         # finished=int(30 * downloader._progress / downloader._total_size)
         # if(len(sys.argv)==4):
         #    if(sys.argv[3]=="bar"):
         #       progress_bar=f"[{'#'*finished}{' '*(30-finished)}]\t"
         ui_update('{1:<35}\t{2:<32}\t{0}'.format(file_name,file_size,percentage))


      fh.close()
      os.rename(os.path.join(location,fileStats['name']+".download"),os.path.join(location,fileStats['name']))
   except Exception as e:
      print(color("!!!   Download Interrupted   !!!\n",Colors.orange))
      print(e)
      print(color("!!!   Download Interrupted   !!!\n",Colors.orange))


def download_folder(folder_id,file_type,select=False):
   """Download Folder
   downloads all the files in the folder
   @params
      folder_id:string
   """
   folder_info=file_info(folder_id)
   download_path=os.path.join(DOWNLOAD_LOCATION,folder_info['name'])
   os.makedirs(download_path,exist_ok=True)
   fileList=list_files_in_folder(folder_id,print_list=False,filterType=file_type,choose=select)

   print(f"================   {folder_info['name']}   ====================\n")
   for file in fileList:
      download_file(file['id'],location=download_path,prompt=False,from_folder=True)

   print("\nLocation: ",download_path)
   print("\n=================================================================")

def list_files():
   """List all files in google drive"""
   results = GOOGLE_DRIVE.files().list(
       pageSize=50, fields="nextPageToken, files(id, name, size)").execute()
   items = results.get('files', [])
   if not items:
        print('No files found.')
   else:
        print('Files:')
        for item in items:
            print('{1:<30}    {0}'.format(item['name'], item['id']))





#######################################################################################################
####################################         UPLOADING         ########################################
#######################################################################################################




def upload_file(file_location,file_parent=""):
   """Upload File
   upload the file to the drive
   also the folder in which you want to upload it
   @params
      file_location:string
      file_parent:string
   """
   
   show_folders_info()
   fileName=Path(file_location).name
   file_metadata={
      "name":fileName,
      "parents:":[file_parent]
   }
   sys.stdout.flush()
   sp= yaspin(text="Uploading...", color="green")
   media = MediaFileUpload(file_location, resumable=True,chunksize=1048576)
   file = GOOGLE_DRIVE.files().create(body=file_metadata,media_body=media,fields='id,name,size,mimeType').execute()
   sp.ok("DONE!")
   # response=None
   # while response is None:
   #  status,done=file.next_chunk()
   #  ui_update('{1:<10}\t{2:<10}\t{0}'.format(fileName,color(convert_size(int(media.size())),Colors.orange),color(f'{round(status.progress()*100,2)}%',Colors.green)))

   print("File Uploaded!!\n",file)


def delete_file(file_id):
    """Delete File
    @params
      file_id:string
    """
    try:
        GOOGLE_DRIVE.files().delete(fileId=file_id).execute()
    except:
        print('An error occurred')



def list_files_in_folder(folder_id,print_list=True,filterType=None,choose=False):
   """List Files in Folder
   prints all the files in the folder with the provided id
   @params
      folder_id:string
      print_list:bool
   """
   results=GOOGLE_DRIVE.files().list(q=f"'{folder_id}' in parents",fields="files(name,id,size,mimeType)",supportsAllDrives=True, includeItemsFromAllDrives=True).execute()
   files=results.get('files')
   token=results.get('nextPageToken')

   while token:
      results=GOOGLE_DRIVE.files().list(q=f"'{folder_id}' in parents",fields="nextPageToken, files(name,id,size,mimeType)",supportsAllDrives=True, includeItemsFromAllDrives=True).execute()
      files.extend(results.get('files'))
      token=results.get('nextPageToken')
   
   if print_list:
      totalSize=0
      total_files=0

      for file in files:
         hasSizeProp='size' in file
         if(filterType!=None):
            if(file['name'].split(".")[-1]!=filterType):
               continue

         total_files+=1
         isFolder=file['mimeType']=='application/vnd.google-apps.folder'
         type="DIR" if isFolder else "-"
         totalSize+=int(file['size']) if hasSizeProp else 0
         if isFolder:
            folder_files=list_files_in_folder(file['id'],print_list=False)
            total_foldersize=0
            for folder in folder_files:
               total_foldersize+=int(folder['size'])

            totalSize+=total_foldersize
            print('{0:<35}{1:>33}  {2}\t{3}'.format(file['id'],color(convert_size(int(total_foldersize)),Colors.orange),color(type,Colors.blue),color(file['name'],Colors.purple)))
         else:
            print('{0:<35}{1:>33}  {2}\t{3}'.format(file['id'],color(convert_size(int(file['size']) if hasSizeProp else 0),Colors.orange),color(type,Colors.blue),color(file['name'],Colors.purple)))

      
      print("=====================================")
      print("\t » Total Files: ",total_files)
      print("\t » Total Size: ",convert_size(totalSize))

   else:
      filter_files=[] if filterType!=None else files
      if(filterType!=None):
         for file in files:
            if(file['mimeType']=='application/vnd.google-apps.folder'):
               continue
            if(filterType!=None):
               if(file['name'].split(".")[-1]!=filterType):
                  continue
            filter_files.append(file)
         if(not choose):
            return filter_files
      if(choose):
         els=[]
         opt_config = {
           'type': 'checkbox',
           'name': 'files',
           'message': 'Files',
           'choices':els 
         }
         for el in filter_files:
            if(el['mimeType']=='application/vnd.google-apps.folder'):
               continue
            els.append({"name":el['name']})

         choice=prompt(opt_config,style=custom_style)
         choosen_one=[]
         for el in choice['files']:
            for file in filter_files:
               if(file['mimeType']=='application/vnd.google-apps.folder'):
                  continue
               if(file['name']==el):
                  choosen_one.append(file)

         return choosen_one
         
      return files
