# Real-Time-Process-Monitor/process_monitor.py
# Aditya Sharma (9) -  Real-Time Updates and Visualization
# Sayan Mondal (8) - GUI
# Rajbardhan Kumar (22) - Process Gathering

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
        self.root.geometry("1000x700")
        
        # Set a more modern theme
        style = ttk.Style()
        available_themes = style.theme_names()
        if 'clam' in available_themes:
            style.theme_use('clam')  # A more modern theme
        
        # Configure custom styles
        style.configure("Treeview", 
                        background="#2e2e2e", 
                        foreground="white",
                        fieldbackground="#2e2e2e", 
                        rowheight=28)
        style.configure("Treeview.Heading", 
                        background="#4a4a4a", 
                        foreground="white", 
                        font=('Helvetica', 10, 'bold'))
        style.map("Treeview", 
                  background=[('selected', '#505050')])
        
        # Configure the main window
        self.root.configure(bg="#1e1e1e")
        self.root.option_add("*Font", "Helvetica 10")
        
        # Create a queue for thread-safe communication
        self.update_queue = Queue()
        
        # Get number of CPU cores
        self.num_cores = psutil.cpu_count()
        
        # Configure grid
        self.root.grid_rowconfigure(1, weight=1)
        self.root.grid_columnconfigure(0, weight=1)
        
        # Create frames with modern styling
        self.info_frame = tk.Frame(root, bg="#1e1e1e", padx=10, pady=10)
        self.info_frame.grid(row=0, column=0, padx=10, pady=10, sticky="ew")
        
        # Create notebook for tabbed interface with modern styling
        self.notebook = ttk.Notebook(root)
        self.notebook.grid(row=1, column=0, padx=10, pady=5, sticky="nsew")
        
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
        
        # Create system info with modern progress bars
        # CPU usage frame
        cpu_frame = tk.Frame(self.info_frame, bg="#1e1e1e")
        cpu_frame.pack(fill=tk.X, pady=5)

        # Labels for CPU and memory should have consistent width
        label_width = 80
        tk.Label(cpu_frame, text="CPU:", fg="white", bg="#1e1e1e", 
                font=("Helvetica", 11, "bold"), width=8, anchor="w").pack(side=tk.LEFT, padx=5)

        # Create a frame for the progress bar
        cpu_bar_frame = tk.Frame(cpu_frame, bg="#1e1e1e", height=30)  # Set a fixed height of 30 pixels
        cpu_bar_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        cpu_bar_frame.pack_propagate(False)  # This prevents the frame from resizing to its contents

        # Progress bar
        self.cpu_progress = ttk.Progressbar(cpu_bar_frame, orient="horizontal", 
                                           length=500, mode="determinate", 
                                           style="TProgressbar")
        self.cpu_progress.pack(fill=tk.BOTH, expand=True)

        # Label inside the bar frame
        self.cpu_label = tk.Label(cpu_bar_frame, text="0%", fg="white", bg="#3498db", 
                                 font=("Helvetica", 10, "bold"))
        self.cpu_label.place(relx=0.5, rely=0.5, anchor="center")

        # Memory usage frame
        mem_frame = tk.Frame(self.info_frame, bg="#1e1e1e")
        mem_frame.pack(fill=tk.X, pady=5)

        tk.Label(mem_frame, text="Memory:", fg="white", bg="#1e1e1e", 
                font=("Helvetica", 11, "bold"), width=8, anchor="w").pack(side=tk.LEFT, padx=5)

        # Create a frame for the memory progress bar
        mem_bar_frame = tk.Frame(mem_frame, bg="#1e1e1e", height=30)  # Same height as CPU bar
        mem_bar_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        mem_bar_frame.pack_propagate(False)  # Prevent the frame from shrinking

        # Progress bar
        self.memory_progress = ttk.Progressbar(mem_bar_frame, orient="horizontal", 
                                              length=500, mode="determinate")
        self.memory_progress.pack(fill=tk.BOTH, expand=True)

        # Label inside the bar frame
        self.memory_label = tk.Label(mem_bar_frame, text="0%", fg="white", bg="#3498db", 
                                    font=("Helvetica", 10, "bold"))
        self.memory_label.place(relx=0.5, rely=0.5, anchor="center")

        # Additional info label for memory
        self.memory_info_label = tk.Label(mem_frame, text="(0 GB free)", fg="white", bg="#1e1e1e", 
                                         font=("Helvetica", 10))
        self.memory_info_label.pack(side=tk.LEFT, padx=5)

        # Customize progress bar colors
        style.configure("TProgressbar", 
                       thickness=25,  # Increase thickness (internal progress bar height)
                       troughcolor="#2e2e2e", 
                       background="#3498db")
        
        # Create search and filter frame
        search_frame = tk.Frame(self.process_frame, bg="#2e2e2e", padx=10, pady=10)
        search_frame.pack(fill=tk.X, pady=(0, 10))
        
        tk.Label(search_frame, text="Search:", fg="white", bg="#2e2e2e").pack(side=tk.LEFT, padx=5)
        self.search_var = tk.StringVar()
        self.search_var.trace("w", self.filter_processes)
        search_entry = tk.Entry(search_frame, textvariable=self.search_var, width=30, 
                              bg="#3a3a3a", fg="white", insertbackground="white")
        search_entry.pack(side=tk.LEFT, padx=5)
        
        # Create process list with improved styling
        columns = ("PID", "Name", "CPU%", "Memory")
        
        # Tree frame with padding
        tree_frame = tk.Frame(self.process_frame, bg="#1e1e1e", padx=5, pady=5)
        tree_frame.pack(fill=tk.BOTH, expand=True)
        
        self.tree = ttk.Treeview(tree_frame, columns=columns, show="headings")
        
        # Configure columns
        self.tree.heading("PID", text="PID", command=lambda: self.sort_processes_by("PID"))
        self.tree.heading("Name", text="Process Name", command=lambda: self.sort_processes_by("Name"))
        self.tree.heading("CPU%", text="CPU %", command=lambda: self.sort_processes_by("CPU%"))
        self.tree.heading("Memory", text="Memory (MB)", command=lambda: self.sort_processes_by("Memory"))
        
        # Configure column widths and anchors
        self.tree.column("PID", width=80, anchor="center")
        self.tree.column("Name", width=300, anchor="w")
        self.tree.column("CPU%", width=100, anchor="center")
        self.tree.column("Memory", width=120, anchor="center")
        
        # Add scrollbar
        scrollbar = ttk.Scrollbar(tree_frame, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        
        # Pack tree and scrollbar
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Create CPU usage visualization
        self.setup_visualization()
        
        # Data for storing history
        self.cpu_history = deque(maxlen=60)  # Store 60 data points
        self.top_processes_history = {}  # Store history for top processes
        self.current_sort = {"column": "CPU%", "reverse": True}  # Default sort
        self.filter_text = ""
        
        # Start the update thread
        self.running = True
        self.update_thread = threading.Thread(target=self.update_loop, daemon=True)
        self.update_thread.start()
        
        # Start checking for updates
        self.check_updates()
    
    def setup_visualization(self):
        """Set up the CPU usage visualization"""
        # Create matplotlib figure with dark style
        plt.style.use('dark_background')
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(8, 6), dpi=100)
        self.fig.tight_layout(pad=3.0)
        self.fig.patch.set_facecolor('#2e2e2e')
        
        # Create the canvas to display the plots
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.viz_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().grid(row=0, column=0, sticky="nsew", padx=10, pady=10)
        
        # Set up the first plot for total CPU usage
        self.ax1.set_title('Total CPU Usage (%)', color='white', fontsize=12)
        self.ax1.set_ylim(0, 100)
        self.ax1.set_xlim(0, 60)
        self.ax1.tick_params(colors='white')
        self.ax1.grid(True, alpha=0.3)
        self.ax1.set_facecolor('#1a1a1a')
        
        # Create empty line for the plot with gradient color
        self.cpu_line, = self.ax1.plot([], [], linewidth=2.5, color='#3498db')
        self.ax1.fill_between([], [], alpha=0.3, color='#3498db')
        
        # Set up the second plot for top 5 processes
        self.ax2.set_title('Top 5 Process CPU Usage (%)', color='white', fontsize=12)
        self.ax2.set_ylim(0, 100)
        self.ax2.set_xlim(0, 60)
        self.ax2.tick_params(colors='white')
        self.ax2.grid(True, alpha=0.3)
        self.ax2.set_facecolor('#1a1a1a')
        
        # Create empty lines for the top 5 processes
        self.process_lines = {}
        self.process_fills = {}
        
        # Use enhanced performance settings
        self.fig.canvas.draw_idle()
        plt.rcParams['path.simplify'] = True
        plt.rcParams['path.simplify_threshold'] = 1.0
    
    def update_visualization(self, cpu_percent, top_processes):
        """Update the CPU usage visualization with better performance"""
        # Update CPU history
        self.cpu_history.append(cpu_percent)
        
        # Update the CPU usage plot
        y_data = list(self.cpu_history)
        x_data = list(range(len(y_data)))
        
        self.cpu_line.set_data(x_data, y_data)
        
        # Update the fill between
        if hasattr(self, 'cpu_fill'):
            self.cpu_fill.remove()
        self.cpu_fill = self.ax1.fill_between(x_data, 0, y_data, alpha=0.2, color='#3498db')
        
        # Only adjust limits when needed
        if len(x_data) > 0 and max(x_data) > self.ax1.get_xlim()[1]:
            self.ax1.set_xlim(0, max(60, max(x_data)))
        
        # Update top processes with smoother handling
        current_pids = set(p['pid'] for p in top_processes)
        
        # Handle processes that are no longer in top 5
        for pid in list(self.process_lines.keys()):
            if pid not in current_pids:
                if pid in self.process_lines:
                    self.process_lines[pid].remove()
                    del self.process_lines[pid]
                if pid in self.process_fills:
                    self.process_fills[pid].remove()
                    del self.process_fills[pid]
                if pid in self.top_processes_history:
                    del self.top_processes_history[pid]
        
        # Update current processes
        legend_changed = False
        colors = ['#e74c3c', '#2ecc71', '#f39c12', '#9b59b6', '#1abc9c']
        
        for i, proc in enumerate(top_processes):
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
                    color = colors[i % len(colors)]
                    line, = self.ax2.plot([], [], label=f"{name[:15]}... (PID: {pid})" if len(name) > 15 else f"{name} (PID: {pid})", 
                                         linewidth=2, color=color)
                    self.process_lines[pid] = line
                    
                    # Add fill beneath
                    fill = self.ax2.fill_between([], [], alpha=0.1, color=color)
                    self.process_fills[pid] = fill
                    
                    legend_changed = True
            
            # Update process history
            self.top_processes_history[pid]['history'].append(cpu)
            
            # Update the line data
            if pid in self.process_lines:
                y_data = list(self.top_processes_history[pid]['history'])
                x_data = list(range(len(y_data)))
                self.process_lines[pid].set_data(x_data, y_data)
                
                # Update fill
                if pid in self.process_fills:
                    self.process_fills[pid].remove()
                    self.process_fills[pid] = self.ax2.fill_between(x_data, 0, y_data, alpha=0.1, color=self.process_lines[pid].get_color())
        
        # Only update legend if it changed
        if legend_changed and self.process_lines:
            self.ax2.legend(loc='upper left', facecolor='#2e2e2e', framealpha=0.9, edgecolor='none')
        
        # Use blit for faster rendering
        self.fig.canvas.draw_idle()
        self.fig.canvas.flush_events()
    
    def filter_processes(self, *args):
        """Filter processes based on search text"""
        self.filter_text = self.search_var.get().lower()
        # The actual filtering happens in check_updates
    
    def sort_processes_by(self, column):
        """Sort processes by the specified column"""
        if self.current_sort["column"] == column:
            self.current_sort["reverse"] = not self.current_sort["reverse"]
        else:
            self.current_sort["column"] = column
            self.current_sort["reverse"] = True
        
        # The actual sorting happens in check_updates
    
    def update_loop(self):
        """Background thread with optimized collection"""
        last_full_update = time.time()
        
        while self.running:
            try:
                # Get system info
                cpu_percent = psutil.cpu_percent(interval=None)
                memory = psutil.virtual_memory()
                
                # Full process scan is expensive, do it less often
                current_time = time.time()
                if current_time - last_full_update >= 2.0:  # Full update every 2 seconds
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
                    last_full_update = current_time
                else:
                    # Just update the existing process list with new CPU values (much faster)
                    if hasattr(self, 'last_processes'):
                        processes = []
                        for proc_info in self.last_processes[:100]:  # Only update top processes
                            try:
                                pid = proc_info['pid']
                                proc = psutil.Process(pid)
                                proc_info['cpu_percent'] = proc.cpu_percent() / self.num_cores
                                processes.append(proc_info)
                            except (psutil.NoSuchProcess, psutil.AccessDenied):
                                pass
                    else:
                        processes = []
                
                # Remember the processes for quick updates
                self.last_processes = processes
                
                # Put the data in the queue
                self.update_queue.put((cpu_percent, memory.percent, processes))
                
                # Sleep for a short time to prevent excessive CPU usage
                time.sleep(0.5)
                
            except Exception as e:
                print(f"Error in update thread: {e}")
                time.sleep(1)
    
    def check_updates(self):
        """Main thread method with optimized updates"""
        try:
            # Check if there's new data
            while not self.update_queue.empty():
                cpu_percent, memory_percent, processes = self.update_queue.get_nowait()
                
                # Update system info
                self.cpu_label.config(text=f"{cpu_percent:.1f}%")
                self.cpu_progress["value"] = cpu_percent

                # Format memory information with available memory
                memory_info = psutil.virtual_memory()
                available_gb = memory_info.available / (1024**3)
                total_gb = memory_info.total / (1024**3)

                self.memory_label.config(text=f"{memory_percent:.1f}%")
                self.memory_info_label.config(text=f"({available_gb:.1f} GB free)")
                self.memory_progress["value"] = memory_percent

                # Adjust label positions inside the progress bars
                cpu_width = self.cpu_progress.winfo_width()
                if cpu_width > 0:  # Only adjust if the widget has been drawn
                    self.cpu_label.place(relx=0.5, rely=0.5, anchor="center")
                    self.memory_label.place(relx=0.5, rely=0.5, anchor="center")
                
                # Filter processes
                if self.filter_text:
                    filtered_processes = [p for p in processes if 
                                        self.filter_text in str(p['pid']).lower() or 
                                        self.filter_text in p['name'].lower()]
                else:
                    filtered_processes = processes
                
                # Sort processes
                column = self.current_sort["column"]
                reverse = self.current_sort["reverse"]
                
                if column == "PID":
                    filtered_processes.sort(key=lambda x: x['pid'], reverse=reverse)
                elif column == "Name":
                    filtered_processes.sort(key=lambda x: x['name'].lower(), reverse=reverse)
                elif column == "CPU%":
                    filtered_processes.sort(key=lambda x: x['cpu_percent'], reverse=reverse)
                elif column == "Memory":
                    filtered_processes.sort(key=lambda x: x['memory_info'].rss if hasattr(x['memory_info'], 'rss') else 0, reverse=reverse)
                
                # Create a map of existing items
                existing_items = {}
                for item in self.tree.get_children():
                    pid = int(self.tree.item(item, 'values')[0])
                    existing_items[pid] = item
                
                # Track which processes we've updated
                updated_pids = set()
                
                # Update or insert process list items (limited to top processes for performance)
                for i, proc in enumerate(filtered_processes[:100]):
                    try:
                        pid = proc['pid']
                        updated_pids.add(pid)
                        memory_mb = proc['memory_info'].rss / (1024 * 1024)  # Convert to MB
                        
                        # Format values nicely
                        cpu_str = f"{proc['cpu_percent']:.1f}%" if proc['cpu_percent'] > 0.1 else "<0.1%"
                        memory_str = f"{memory_mb:.1f}"
                        
                        # If PID exists, update it
                        if pid in existing_items:
                            self.tree.item(existing_items[pid], values=(
                                pid,
                                proc['name'],
                                cpu_str,
                                memory_str
                            ))
                        else:
                            # Otherwise insert new item
                            self.tree.insert("", i, values=(
                                pid,
                                proc['name'],
                                cpu_str,
                                memory_str
                            ))
                    except (KeyError, AttributeError) as e:
                        continue
                
                # Remove items no longer in the process list
                for pid, item in existing_items.items():
                    if pid not in updated_pids:
                        self.tree.delete(item)
                
                # Update visualization (only take top 5 processes for the chart)
                top_processes = filtered_processes[:5]
                self.update_visualization(cpu_percent, top_processes)
                
        except Exception as e:
            print(f"Error updating UI: {e}")
        
        # Schedule next check - longer interval
        self.root.after(500, self.check_updates)
    
    def __del__(self):
        """Cleanup when the window is closed"""
        self.running = False
        if hasattr(self, 'update_thread'):
            self.update_thread.join(timeout=1)

def main():
    root = tk.Tk()
    app = ProcessMonitor(root)
    root.protocol("WM_DELETE_WINDOW", lambda: (setattr(app, 'running', False), root.destroy()))
    root.mainloop()

if __name__ == "__main__":
    main()