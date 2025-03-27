import tkinter as tk
from tkinter import ttk
import psutil
import time
from datetime import datetime
import threading
from queue import Queue

class ProcessMonitor:
    def __init__(self, root):
        self.root = root
        self.root.title("Process Monitor")
        self.root.geometry("800x600")
        
        # Create a queue for thread-safe communication
        self.update_queue = Queue()
        
        # Get number of CPU cores
        self.num_cores = psutil.cpu_count()
        
        # Configure grid
        self.root.grid_rowconfigure(1, weight=1)
        self.root.grid_columnconfigure(0, weight=1)
        
        # Create frames
        self.info_frame = ttk.Frame(root)
        self.info_frame.grid(row=0, column=0, padx=5, pady=5, sticky="ew")
        
        self.process_frame = ttk.Frame(root)
        self.process_frame.grid(row=1, column=0, padx=5, pady=5, sticky="nsew")
        
        # Configure process frame grid
        self.process_frame.grid_rowconfigure(0, weight=1)
        self.process_frame.grid_columnconfigure(0, weight=1)
        
        # Create system info labels
        self.cpu_label = ttk.Label(self.info_frame, text="Total CPU Usage: 0%")
        self.cpu_label.pack(side=tk.LEFT, padx=5)
        
        self.memory_label = ttk.Label(self.info_frame, text="Memory Usage: 0%")
        self.memory_label.pack(side=tk.LEFT, padx=5)
        
        # Create and configure style for Treeview
        style = ttk.Style()
        style.configure("Treeview", rowheight=25)  # Increase row height for better visibility
        style.configure("Treeview.Heading", font=('Helvetica', 10, 'bold'))
        
        # Create process list
        self.tree = ttk.Treeview(self.process_frame, columns=("PID", "Name", "CPU%", "Memory"), 
                                show="headings", style="Treeview")
        self.tree.heading("PID", text="PID")
        self.tree.heading("Name", text="Name")
        self.tree.heading("CPU%", text="CPU%")
        self.tree.heading("Memory", text="Memory (MB)")
        
        # Configure column widths and add borders
        self.tree.column("PID", width=100, anchor="center")
        self.tree.column("Name", width=300, anchor="w")
        self.tree.column("CPU%", width=100, anchor="center")
        self.tree.column("Memory", width=100, anchor="center")
        
        # Add scrollbar
        scrollbar = ttk.Scrollbar(self.process_frame, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        
        # Grid the tree and scrollbar
        self.tree.grid(row=0, column=0, sticky="nsew")
        scrollbar.grid(row=0, column=1, sticky="ns")
        
        # Start the update thread
        self.running = True
        self.update_thread = threading.Thread(target=self.update_loop, daemon=True)
        self.update_thread.start()
        
        # Start checking for updates
        self.check_updates()
    
    def update_loop(self):
        """Background thread that collects system and process information"""
        while self.running:
            try:
                # Get system info
                cpu_percent = psutil.cpu_percent(interval=None)
                memory = psutil.virtual_memory()
                
                # Get process list
                processes = []
                for proc in psutil.process_iter(['pid', 'name', 'cpu_percent', 'memory_info']):
                    try:
                        pinfo = proc.info
                        # Skip System Idle Process and convert per-core percentage to system-wide percentage
                        if pinfo['name'].lower() not in ['system idle process', 'idle']:
                            pinfo['cpu_percent'] = pinfo['cpu_percent'] / self.num_cores
                            processes.append(pinfo)
                    except (psutil.NoSuchProcess, psutil.AccessDenied):
                        continue
                
                # Sort by CPU usage
                processes.sort(key=lambda x: x['cpu_percent'], reverse=True)
                
                # Put the data in the queue
                self.update_queue.put((cpu_percent, memory.percent, processes))
                
                # Sleep for a short time to prevent excessive CPU usage
                time.sleep(0.5)
                
            except Exception as e:
                print(f"Error in update thread: {e}")
                time.sleep(1)
    
    def check_updates(self):
        """Main thread method to check for and apply updates"""
        try:
            # Check if there's new data
            while not self.update_queue.empty():
                cpu_percent, memory_percent, processes = self.update_queue.get_nowait()
                
                # Update system info
                self.cpu_label.config(text=f"Total CPU Usage: {cpu_percent:.1f}% (Available: {100-cpu_percent:.1f}%)")
                self.memory_label.config(text=f"Memory Usage: {memory_percent:.1f}%")
                
                # Clear existing items
                for item in self.tree.get_children():
                    self.tree.delete(item)
                
                # Update process list
                for proc in processes:
                    try:
                        memory_mb = proc['memory_info'].rss / (1024 * 1024)  # Convert to MB
                        self.tree.insert("", "end", values=(
                            proc['pid'],
                            proc['name'],
                            f"{proc['cpu_percent']:.1f}",
                            f"{memory_mb:.1f}"
                        ))
                    except (KeyError, AttributeError):
                        continue
                
        except Exception as e:
            print(f"Error updating UI: {e}")
        
        # Schedule next check
        self.root.after(100, self.check_updates)
    
    def __del__(self):
        """Cleanup when the window is closed"""
        self.running = False
        if hasattr(self, 'update_thread'):
            self.update_thread.join(timeout=1)

def main():
    root = tk.Tk()
    app = ProcessMonitor(root)
    root.mainloop()

if __name__ == "__main__":
    main() 