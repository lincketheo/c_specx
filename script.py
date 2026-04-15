import os

# Configuration
OLD_NAME = "c_specx"
NEW_NAME = "c_specx"
EXCLUDE_DIRS = {'.git', '.vscode', '.DS_Store', 'build', 'bin'}

def update_content():
    print(f"--- Updating file contents: {OLD_NAME} -> {NEW_NAME} ---")
    for root, dirs, files in os.walk(".", topdown=True):
        # Skip excluded directories
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        
        for file in files:
            file_path = os.path.join(root, file)
            try:
                with open(file_path, 'rb') as f:
                    content = f.read()
                
                # Check if the old name exists in bytes to avoid unnecessary writes
                old_bytes = OLD_NAME.encode('utf-8')
                new_bytes = NEW_NAME.encode('utf-8')
                
                if old_bytes in content:
                    print(f"Updating content: {file_path}")
                    new_content = content.replace(old_bytes, new_bytes)
                    with open(file_path, 'wb') as f:
                        f.write(new_content)
            except Exception as e:
                print(f"Skipping {file_path} due to error: {e}")

def rename_files_and_dirs():
    print(f"\n--- Renaming files and folders: {OLD_NAME} -> {NEW_NAME} ---")
    # topdown=False is critical: it renames children before parents
    for root, dirs, files in os.walk(".", topdown=False):
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        
        for name in files + dirs:
            if OLD_NAME in name:
                old_path = os.path.join(root, name)
                new_name = name.replace(OLD_NAME, NEW_NAME)
                new_path = os.path.join(root, new_name)
                
                print(f"Renaming: {old_path} -> {new_path}")
                os.rename(old_path, new_path)

if __name__ == "__main__":
    # 1. Change text inside files first
    update_content()
    # 2. Rename the files and folders themselves
    rename_files_and_dirs()
    print("\nDone! Your library is now c_specx.")
