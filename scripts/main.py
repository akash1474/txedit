"""Install Dependencies:
langchain-community               0.3.15
langchain-core                    0.3.31
langchain-google-genai            2.0.9
sentence-transformers             3.3.1
cromadb
"""

import os
import time
import json
from datetime import datetime
import shutil

from langchain.text_splitter import CharacterTextSplitter
from langchain_community.document_loaders import TextLoader
from langchain_huggingface import HuggingFaceEmbeddings
from langchain_chroma import Chroma
from langchain_google_genai import ChatGoogleGenerativeAI
from langchain.prompts import PromptTemplate
from langchain.chains import LLMChain
from langchain.memory import ConversationBufferMemory
from langchain.schema import HumanMessage, AIMessage

from fastapi import FastAPI, Request
from fastapi.responses import StreamingResponse

from concurrent.futures import ThreadPoolExecutor, as_completed


# Define directories
PROJECT_DIR = r"D:/Projects/c++/ask-llm"
DB_DIR = os.path.join(PROJECT_DIR, ".cache/db")
MEMORY_FILE = "chat_history.json"
IGNORED_DIRECTORIES=[]
FILE_EXTENSIONS=[]


def check_file_update(file_path, file_db_dir):
    """
    Checks if the vector store for the given file needs to be updated based on its modification time.
    Returns True if the vector store needs to be updated, False otherwise.
    """
    metadata_file_path = os.path.join(file_db_dir, "metadata.txt")
    try:
        last_modified_time = datetime.fromtimestamp(
            os.stat(file_path).st_mtime
        ).isoformat()
    except FileNotFoundError:
        print(f"File {file_path} not found. Skipping...")
        return False, None

    if os.path.exists(metadata_file_path):
        try:
            with open(metadata_file_path, "r") as meta_file:
                stored_metadata = meta_file.read().strip()
            if stored_metadata == last_modified_time:
                print(f"No changes detected for {file_path}. Skipping...")
                return False, None
            print(f"Changes detected for {file_path}. Updating vector store...")
            shutil.rmtree(file_db_dir)
        except FileNotFoundError:
            print(
                f"Metadata or vector store not found for {file_path}. Reinitializing..."
            )
    else:
        print(f"Creating vector store for {file_path}...")

    return True, last_modified_time


"""
Returns a summarized project structure up to a certain depth.
"""
def get_project_structure(root_dir, max_depth=2):
    structure = []

    def walk_dir(current_dir, depth):
        if depth > max_depth:
            return
        for entry in os.scandir(current_dir):
            if entry.is_file():
                structure.append(
                    {"type": "file", "path": os.path.relpath(entry.path, root_dir)}
                )
            elif entry.is_dir():
                structure.append(
                    {"type": "directory", "path": os.path.relpath(entry.path, root_dir)}
                )
                walk_dir(entry.path, depth + 1)

    walk_dir(root_dir, 0)
    return structure


import os


def create_vector_store_for_file(file_path):
    global DB_DIR 
    # Name the vector store directory using file path and filename
    file_db_name = os.path.basename(
        file_path
    )  # md5(file_path.encode()).hexdigest() + "_" +
    file_db_dir = os.path.join(DB_DIR, file_db_name)

    # Check if the vector store needs to be updated
    needs_update, last_modified_time = check_file_update(file_path, file_db_dir)
    if not needs_update:
        return

    # Load and process the file
    try:
        loader = TextLoader(file_path)
        documents = loader.load()
    except Exception as e:
        print(f"Error loading file {file_path}: {e}")
        return

    for doc in documents:
        doc.metadata = {"source": os.path.relpath(file_path, PROJECT_DIR)}

    # Split the documents into chunks
    text_splitter = CharacterTextSplitter(chunk_size=1000, chunk_overlap=100)
    docs = text_splitter.split_documents(documents)

    try:
        # Initialize embeddings and create the vector store
        embeddings = HuggingFaceEmbeddings(
            model_name="sentence-transformers/all-MiniLM-L6-v2"
        )
        db = Chroma.from_documents(docs, embeddings, persist_directory=file_db_dir)
    except Exception as e:
        print(f"Error creating vector store for {file_path}: {e}")
        if os.path.exists(file_db_dir):
            shutil.rmtree(file_db_dir)  # Ensure no partial vector store remains
            return

    # Save the file's metadata
    os.makedirs(file_db_dir, exist_ok=True)
    with open(os.path.join(file_db_dir, "metadata.txt"), "w") as meta_file:
        meta_file.write(last_modified_time)

    # print(f"Vector store for {file_path} created successfully.")


def initialize_vector_store():
    """
    Initializes a separate Chroma vector store for each file in the project directory.
    """
    print(f"Projects directory: {PROJECT_DIR}")
    print(f"Persistent directory: {DB_DIR}")

    # Ensure the persistent directory exists
    os.makedirs(DB_DIR, exist_ok=True)

    project_files = []
    for root, dirs, files in os.walk(PROJECT_DIR):
        dirs[:] = [d for d in dirs if d not in IGNORED_DIRECTORIES]

        for file in files:
            if file == "__init__.py":
                continue
            if file.endswith(tuple(FILE_EXTENSIONS)):
                project_files.append(os.path.join(root, file))

    max_threads = os.cpu_count()
    print(f"Using {max_threads} Threads")
    with ThreadPoolExecutor(max_workers=max_threads) as executor:
        results = executor.map(create_vector_store_for_file, project_files)


def query_vector_store(user_query):
    """
    Queries all vector stores concurrently and retrieves relevant documents based on the user query.
    """
    print("\n--- Querying all vector stores ---")
    relevant_results = []

    def query_single_store(file_db_dir):
        try:
            embeddings = HuggingFaceEmbeddings(
                model_name="sentence-transformers/all-MiniLM-L6-v2"
            )
            db = Chroma(persist_directory=file_db_dir, embedding_function=embeddings)

            retriever = db.as_retriever(
                search_type="similarity", search_kwargs={"k": 3}
            )
            return retriever.invoke(user_query)
        except Exception as e:
            print(f"Error querying vector store in {file_db_dir}: {e}")
            return []

    # Get all valid directories for querying
    vector_store_dirs = [
        os.path.join(DB_DIR, dir_name)
        for dir_name in os.listdir(DB_DIR)
        if os.path.isdir(os.path.join(DB_DIR, dir_name))
    ]

    with ThreadPoolExecutor(max_workers=None) as executor:
        future_to_dir = {
            executor.submit(query_single_store, dir_name): dir_name
            for dir_name in vector_store_dirs
        }

        for future in as_completed(future_to_dir):
            relevant_docs = future.result()
            if relevant_docs:
                relevant_results.extend(relevant_docs)

    # Display relevant results
    # if not relevant_results:
    #     print("No relevant documents found.")
    # else:
    #     print("\n--- Relevant Documents ---")
    #     for i, doc in enumerate(relevant_results, 1):
    #         print(f"Source:{doc.metadata['source']} Document {i}")

    return relevant_results


def track_token_usage(prompt, response, llm):
    """
    Tracks and displays token usage for the prompt and response.
    """
    prompt_tokens = llm.get_num_tokens(prompt)
    response_tokens = llm.get_num_tokens(
        response.content if hasattr(response, "content") else response
    )
    total_tokens = prompt_tokens + response_tokens

    print("\n--- Token Usage ---")
    print(f"Prompt Tokens: {prompt_tokens}")
    print(f"Response Tokens: {response_tokens}")
    print(f"Total Tokens: {total_tokens}")


def generate_llm_response(relevant_docs, user_query, memory,use_vector):
    """Generates a response using Google Gemini and maintains chat history."""

    context = "\n\n".join([doc.page_content for doc in relevant_docs])
    project_structure = get_project_structure(PROJECT_DIR)

    # Initialize Gemini LLM
    llm = ChatGoogleGenerativeAI(
        model="gemini-2.0-flash",
        api_key="AIzaSyA7FTjQca-Zvt3DQkmmmu-ItWI6UuBRu7I"
    )

    input_data={}
    history_text=""
    for msg in memory.chat_memory.messages:
    	history_text+=f"Question: {msg.content}\n" if isinstance(msg, HumanMessage) else f"Answer: {msg.content}\n"

    # Define prompt structure
    input_variables=[]
    template=()
    if use_vector:
        input_variables=["context", "query", "project_structure", "history"]
        template=(
            "You are a project assistant. Based on the following project details and conversation history, answer the query accurately:\n\n"
            "Conversation History:\n{history}\n\n Conversation History Ended"
            "\n\nCurrent Conversation Details Starts from here\n\n"
            "Project Details:{project_structure}\n\n{context}\n\n"
            "User Query:\n{query}\n\n"
        )

        input_data = {
            "context": context,
            "query": user_query,
            "history": history_text,
            "project_structure": project_structure  # Replace with actual project details if needed
        }
    else:
        input_variables=["query","history"]
        template=(
            "Conversation History:\n{history}\n\n Conversation History Ended"
            "Current Query:\n{query}\n\n"
        )

        input_data = {
            "query": user_query,
            "history":history_text
        }

    prompt_template=PromptTemplate(input_variables=input_variables,template=template)

    chain= prompt_template | llm
    # chain = LLMChain(llm=llm, prompt=prompt_template, memory=memory)
    return [chain,input_data]  # Returns a full response as a string



app = FastAPI()

def load_memory():
    """Loads conversation history from a JSON file and converts it to LangChain format."""
    print(f"Loading Memory:{MEMORY_FILE}")
    if os.path.exists(MEMORY_FILE):
        with open(MEMORY_FILE, "r") as file:
            messages = json.load(file)

        # Convert JSON back to LangChain message objects
        return [HumanMessage(content=msg["content"]) if msg["type"] == "human" else AIMessage(content=msg["content"]) for msg in messages]
    
    return []

def update_memory(user_query, model_response):
	print(f"Updating Memory:{MEMORY_FILE}")
	memory.chat_memory.messages.append(HumanMessage(content=user_query))
	memory.chat_memory.messages.append(AIMessage(content=model_response))
	save_memory(memory)  # Save to disk

def save_memory(memory):
    """Saves conversation history to a JSON file in a serializable format."""
    print(f"Saving Memory:{MEMORY_FILE}")
    messages = []
    for msg in memory.chat_memory.messages:
    	messages.append({
    		"type": "human" if isinstance(msg, HumanMessage) else "ai",
    		"content": msg.content
    	})

    with open(MEMORY_FILE, "w") as file:
        json.dump(messages, file,indent=4)

# memory_data = load_memory()
memory = ConversationBufferMemory()
# memory.chat_memory.messages = memory_data  # Restore previous chat

async def generate_response(query, use_vector):
    """Handles streaming response for the chat API."""
    
    global memory

    relevant_docs = []
    if use_vector:
        yield json.dumps({"type": "msg", "content": "Querying Vector Store"}) + "\n"
        relevant_docs = query_vector_store(query)

    yield json.dumps({"type": "msg", "content": "Making request to Gemini"}) + "\n"

    res_obj = generate_llm_response(relevant_docs, query, memory,use_vector)
    chain=res_obj[0];
    input_data=res_obj[1];

    yield json.dumps({"type": "msg", "content": "Streaming Response"}) + "\n"
    response="";
    for token in chain.stream(input_data):
        response+=token.content
        yield json.dumps({"type": "response", "content": token.content, "done": False}) + "\n"

    # Finalize streaming
    time.sleep(0.2);
    yield json.dumps({"type":"response" ,"content": "", "done": True}) + "\n"
    update_memory(query,response)


@app.post("/api/chat")
async def stream_chat_response(request: Request):
    req_body = await request.json()
    query = req_body.get("query", "")
    use_vector = req_body.get("use_vector", False)

    return StreamingResponse(generate_response(query,use_vector), media_type="application/json")

def generate_response_init():
    yield json.dumps({"type": "msg", "content": "Generating Vector Store"}) + "\n"
    initialize_vector_store();


@app.post("/api/chat/init")
async def setup_chat(request: Request):
    req_body = await request.json()
    global IGNORED_DIRECTORIES
    global FILE_EXTENSIONS
    global PROJECT_DIR
    global DB_DIR
    global MEMORY_FILE
    global memory
    IGNORED_DIRECTORIES = req_body.get("ignored_folders", [])
    FILE_EXTENSIONS = req_body.get("file_extensions", [])
    PROJECT_DIR = req_body.get("project_dir", "")
    DB_DIR = os.path.join(PROJECT_DIR, ".cache/db")
    MEMORY_FILE=os.path.join(PROJECT_DIR,".cache/chat_history.json")
    print(req_body)

    print("[LOG]: Loading Memory")
    memory_data = load_memory()
    memory = ConversationBufferMemory()
    memory.chat_memory.messages = memory_data  # Restore previous chat
    print("[LOG]: Memory Loaded")

    return StreamingResponse(generate_response_init(), media_type="application/json")

"""
uvicorn scripts.main:app --host 0.0.0.0 --port 8000 --reload --reload-dir scripts
"""