import tkinter as tk
from tkinter import ttk
import psutil
import time
from datetime import datetime
import threading
from queue import Queue
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np
from collections import deque

class ProcessMonitor:
    def __init__(self, root):
        self.root = root
        self.root.title("Process Monitor")
        self.root.geometry("900x650")
        
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
        
        # Create notebook for tabbed interface
        self.notebook = ttk.Notebook(root)
        self.notebook.grid(row=1, column=0, padx=5, pady=5, sticky="nsew")
        
        # Create process list tab
        self.process_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.process_frame, text="Processes")
        
        # Create visualization tab
        self.viz_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.viz_frame, text="CPU Visualization")
        
        # Configure process frame grid
        self.process_frame.grid_rowconfigure(0, weight=1)
        self.process_frame.grid_columnconfigure(0, weight=1)
        
        # Configure visualization frame grid
        self.viz_frame.grid_rowconfigure(0, weight=1)
        self.viz_frame.grid_columnconfigure(0, weight=1)
        
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
        
        # Create CPU usage visualization
        self.setup_visualization()
        
        # Data for storing history
        self.cpu_history = deque(maxlen=60)  # Store 60 data points (30 seconds at 0.5s interval)
        self.top_processes_history = {}  # Store history for top processes
        
        # Start the update thread
        self.running = True
        self.update_thread = threading.Thread(target=self.update_loop, daemon=True)
        self.update_thread.start()
        
        # Start checking for updates
        self.check_updates()
    
    def setup_visualization(self):
        """Set up the CPU usage visualization"""
        # Create matplotlib figure
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(8, 6), dpi=100)
        self.fig.tight_layout(pad=3.0)
        
        # Create the canvas to display the plots
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.viz_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().grid(row=0, column=0, sticky="nsew")
        
        # Set up the first plot for total CPU usage
        self.ax1.set_title('Total CPU Usage (%)')
        self.ax1.set_ylim(0, 100)
        self.ax1.set_xlim(0, 60)
        self.ax1.grid(True)
        
        # Create empty line for the plot
        self.cpu_line, = self.ax1.plot([], [], 'b-', linewidth=2)
        
        # Set up the second plot for top 5 processes
        self.ax2.set_title('Top 5 Process CPU Usage (%)')
        self.ax2.set_ylim(0, 100)
        self.ax2.set_xlim(0, 60)
        self.ax2.grid(True)
        
        # Create empty lines for the top 5 processes
        self.process_lines = {}
        self.process_labels = {}
        
    def update_visualization(self, cpu_percent, top_processes):
        """Update the CPU usage visualization"""
        # Update CPU history
        self.cpu_history.append(cpu_percent)
        
        # Update the CPU usage plot
        y_data = list(self.cpu_history)
        x_data = list(range(len(y_data)))
        
        self.cpu_line.set_data(x_data, y_data)
        
        if len(x_data) > 0:
            self.ax1.set_xlim(0, max(60, len(x_data)))
        
        # Update top processes
        for proc in top_processes:
            pid = proc['pid']
            cpu = proc['cpu_percent']
            name = proc['name']
            
            # Initialize history for new processes
            if pid not in self.top_processes_history:
                self.top_processes_history[pid] = {
                    'history': deque([0] * len(self.cpu_history), maxlen=60),
                    'name': name
                }
                
                # Create a new line if needed
                if pid not in self.process_lines:
                    color = plt.cm.tab10(len(self.process_lines) % 10)
                    line, = self.ax2.plot([], [], label=f"{name} (PID: {pid})", linewidth=1.5, color=color)
                    self.process_lines[pid] = line
            
            # Update process history
            self.top_processes_history[pid]['history'].append(cpu)
        
        # Remove processes that are no longer in top 5
        current_pids = [p['pid'] for p in top_processes]
        for pid in list(self.process_lines.keys()):
            if pid not in current_pids:
                if pid in self.process_lines:
                    self.process_lines[pid].remove()
                    del self.process_lines[pid]
                if pid in self.top_processes_history:
                    del self.top_processes_history[pid]
        
        # Update the process usage plots
        for pid, data in self.top_processes_history.items():
            if pid in self.process_lines:
                y_data = list(data['history'])
                x_data = list(range(len(y_data)))
                self.process_lines[pid].set_data(x_data, y_data)
        
        # Update the legend
        if self.process_lines:
            self.ax2.legend(loc='upper left')
        
        # Redraw the canvas
        self.canvas.draw()
    
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
                
                # Update visualization (only take top 5 processes for the chart)
                top_processes = processes[:5]
                self.update_visualization(cpu_percent, top_processes)
                
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