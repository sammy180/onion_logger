#include <iostream>
#include <libudev.h>
#include <gtk/gtk.h>
#include <string>
#include <sys/stat.h>
#include <fstream>
#include <cstdlib>

// Function to check if a symlink (like /dev/OnionX) exists
bool symlink_exists(const std::string &symlink_path) {
    struct stat buffer;
    return (lstat(symlink_path.c_str(), &buffer) == 0);
}

// Function to show a popup with relabel options
std::string show_relabel_popup(const std::string &device_name, bool is_existing) {
    GtkWidget *dialog;
    GtkWidget *combo;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;

    gtk_init(0, nullptr);

    // Create the popup dialog
    dialog = gtk_dialog_new_with_buttons(
        "Device Registration",
        nullptr,
        flags,
        "_OK",
        GTK_RESPONSE_OK,
        "_Cancel",
        GTK_RESPONSE_CANCEL,
        NULL
    );

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    if (is_existing) {
        label = gtk_label_new("Device already registered. Would you like to relabel?");
    } else {
        label = gtk_label_new("New device detected. Select a label for registration:");
    }

    // Create combo box for Onion1 - Onion4
    combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Onion1");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Onion2");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Onion3");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Onion4");

    gtk_container_add(GTK_CONTAINER(content_area), label);
    gtk_container_add(GTK_CONTAINER(content_area), combo);
    gtk_widget_show_all(dialog);

    // Run the dialog and capture the response
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    std::string selected_label;

    if (response == GTK_RESPONSE_OK) {
        selected_label = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
        std::cout << "Selected label: " << selected_label << "\n";
    } else {
        std::cout << "User canceled the operation.\n";
    }

    gtk_widget_destroy(dialog);
    
    return selected_label;
}

// Function to register or relabel the device
void register_or_relabel_device(const std::string &vendor, const std::string &product, const std::string &label) {
    // Create udev rule string
    std::string udev_rule = "SUBSYSTEM==\"tty\", ATTRS{idVendor}==\"" + vendor + "\", ATTRS{idProduct}==\"" + product + "\", SYMLINK+=\"" + label + "\"\n";

    // Append the rule to udev rules file
    std::ofstream udev_file("/etc/udev/rules.d/99-usb-serial.rules", std::ios::app);
    if (udev_file.is_open()) {
        udev_file << udev_rule;
        udev_file.close();
    } else {
        std::cerr << "Error: Could not open /etc/udev/rules.d/99-usb-serial.rules" << std::endl;
    }

    // Reload udev rules
    std::system("sudo udevadm control --reload-rules");
    std::system("sudo udevadm trigger");
}

// Function to listen for USB device connections
void monitor_usb() {
    struct udev *udev = udev_new();
    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", "usb_device");
    udev_monitor_enable_receiving(mon);
    
    int fd = udev_monitor_get_fd(mon);
    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        
        if (select(fd + 1, &fds, nullptr, nullptr, nullptr) > 0 && FD_ISSET(fd, &fds)) {
            struct udev_device *dev = udev_monitor_receive_device(mon);
            if (dev) {
                const char *action = udev_device_get_action(dev);
                const char *vendor = udev_device_get_sysattr_value(dev, "idVendor");
                const char *product = udev_device_get_sysattr_value(dev, "idProduct");
                if (action && std::string(action) == "add") {
                    std::string device_name = "USB Device (Vendor: " + std::string(vendor) + ", Product: " + std::string(product) + ")";
                    
                    // Check for existing symlinks "Onion1" to "Onion4"
                    std::string existing_label;
                    for (int i = 1; i <= 4; i++) {
                        std::string symlink = "/dev/Onion" + std::to_string(i);
                        if (symlink_exists(symlink)) {
                            std::cout << "Device already registered as " << symlink << ".\n";
                            existing_label = symlink;
                            break;
                        }
                    }

                    // Show the relabel/registration pop-up
                    bool is_existing = !existing_label.empty();
                    std::string new_label = show_relabel_popup(device_name, is_existing);

                    // If a label was selected, proceed with relabel or registration
                    if (!new_label.empty()) {
                        register_or_relabel_device(vendor, product, new_label);
                    }
                }
                udev_device_unref(dev);
            }
        }
    }

    udev_unref(udev);
}

int main() {
    monitor_usb();
    return 0;
}

//g++ usb.cpp -o usb `pkg-config --cflags --libs gtk+-3.0` -ludev

