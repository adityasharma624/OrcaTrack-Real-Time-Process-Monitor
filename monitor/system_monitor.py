import psutil

def get_cpu_usage():
    """Returns the current CPU usage percentage."""
    return psutil.cpu_percent(interval=1)

def get_cpu_per_core():
    """Returns a list of CPU usage percentages for each core."""
    return psutil.cpu_percent(interval=1, percpu=True)

def get_memory_usage():
    """Returns memory usage statistics as a dictionary."""
    mem = psutil.virtual_memory()
    return {
        "total": mem.total,
        "used": mem.used,
        "available": mem.available,
        "percent": mem.percent
    }

def get_disk_usage():
    """Returns disk usage statistics for the main partition."""
    disk = psutil.disk_usage('/')
    return {
        "total": disk.total,
        "used": disk.used,
        "free": disk.free,
        "percent": disk.percent
    }

def get_network_stats():
    """Returns network I/O statistics (bytes sent/received)."""
    net_io = psutil.net_io_counters()
    return {
        "bytes_sent": net_io.bytes_sent,
        "bytes_received": net_io.bytes_recv
    }

if __name__ == "__main__":
    print("CPU Usage:", get_cpu_usage(), "%")
    print("CPU Per Core:", get_cpu_per_core())
    print("Memory Usage:", get_memory_usage())
    print("Disk Usage:", get_disk_usage())
    print("Network Stats:", get_network_stats())
