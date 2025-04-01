/*
 * Energy Provider System
 * COMP-3400 Final Project
 * 
 * Tracks customers, energy usage, billing, and payment reminders
 * for an energy company with 500 customers across 5 provinces.
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>

using namespace std;

// Energy types our company provides
enum class EnergyType {
    CRUDE_OIL,
    SOLAR,
    NUCLEAR,
    NATURAL_GAS
};

// Convert energy type to readable string - needed for reports
string getEnergyName(EnergyType type) {
    switch (type) {
        case EnergyType::CRUDE_OIL: return "Crude Oil";
        case EnergyType::SOLAR: return "Solar";
        case EnergyType::NUCLEAR: return "Nuclear";
        case EnergyType::NATURAL_GAS: return "Natural Gas";
        default: return "Unknown";
    }
}

// Keeps track of billing info
struct Payment {
    double amount;
    time_t date;
    bool isPaid;

    Payment(double amt) : amount(amt), isPaid(false) {
        // Use current time as billing date
        date = time(nullptr);
    }
    
    // How many days since we sent the bill
    int getDaysSince() const {
        time_t now = time(nullptr);
        return static_cast<int>(difftime(now, date) / (60 * 60 * 24));
    }
    
    // Past 30 days and still not paid? It's overdue
    bool isOverdue() const {
        return getDaysSince() > 30 && !isPaid;
    }
    
    // Nicer date format for printing
    string formatDate() const {
        struct tm *timeinfo = localtime(&date);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
        return string(buffer);
    }
};

// For tracking our imports and exports
struct ImportExport {
    EnergyType energyType;
    double quantity; // units depend on energy type
    double price;    // per unit
    bool isImport;   // true=import, false=export
    time_t date;
    
    ImportExport(EnergyType type, double qty, double pr, bool import) 
        : energyType(type), quantity(qty), price(pr), isImport(import) {
        date = time(nullptr);
    }
    
    // Total value of this transaction
    double getTotalValue() const {
        return quantity * price;
    }
    
    // Format date for reports
    string formatDate() const {
        struct tm *timeinfo = localtime(&date);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
        return string(buffer);
    }
};

// Our customer class
class Customer {
private:
    int customerID;
    string name;
    string province;
    string email;
    string address;
    EnergyType primaryEnergyType;
    
    double energyAllocated;    // How much they're allowed to use
    double energyUsed;         // Used so far this period
    
    vector<Payment> paymentHistory;
    bool reminderSent;
    
    // For tracking maintenance stuff
    struct MaintenanceRecord {
        time_t date;
        string description;
        double cost;
    };
    vector<MaintenanceRecord> maintenanceHistory;

public:
    // Constructor with all the needed info
    Customer(int id, string name, string province, string email, string address, 
             EnergyType type, double allocated)
        : customerID(id), name(name), province(province), email(email), address(address),
          primaryEnergyType(type), energyAllocated(allocated), energyUsed(0.0), 
          reminderSent(false) {}

    // Record energy usage - returns false if they try to use too much
    bool useEnergy(double amount) {
        if (amount <= getEnergyLeft()) {
            energyUsed += amount;
            return true;
        } else {
            return false;
        }
    }

    // Create a new bill based on current usage
    void createBill(double rate) {
        double amount = energyUsed * rate;
        paymentHistory.push_back(Payment(amount));
        
        // Reset for next month
        energyUsed = 0.0;
    }
    
    // Process a payment for a specific bill
    bool makePayment(int billIndex, double amount) {
        if (billIndex >= 0 && billIndex < paymentHistory.size()) {
            if (amount >= paymentHistory[billIndex].amount) {
                paymentHistory[billIndex].isPaid = true;
                reminderSent = false;
                return true;
            }
        }
        return false;
    }
    
    // Add maintenance work to customer's record
    void addMaintenance(const string& description, double cost) {
        MaintenanceRecord record;
        record.date = time(nullptr);
        record.description = description;
        record.cost = cost;
        maintenanceHistory.push_back(record);
    }
    
    // Generate email reminder text for overdue bills
    string sendReminder() {
        if (hasOverdueBills() && !reminderSent) {
            reminderSent = true;
            
            stringstream email;
            email << "To: " << this->email << "\n"
                  << "Subject: Your energy payment is overdue\n\n"
                  << "Hi " << name << ",\n\n"
                  << "Just a reminder that you have unpaid bills that are now overdue:\n\n";
            
            for (size_t i = 0; i < paymentHistory.size(); i++) {
                const Payment& payment = paymentHistory[i];
                if (payment.isOverdue()) {
                    email << "Bill from " << payment.formatDate()
                          << " - Amount: $" << fixed << setprecision(2) << payment.amount
                          << " - " << payment.getDaysSince() - 30 << " days overdue\n";
                }
            }
            
            email << "\nPlease pay ASAP to avoid service interruption.\n\n"
                  << "Thanks,\nCustomer Service Team";
            
            return email.str();
        }
        return "";
    }

    // Various getters
    int getID() const { return customerID; }
    string getName() const { return name; }
    string getProvince() const { return province; }
    string getEmail() const { return email; }
    EnergyType getEnergyType() const { return primaryEnergyType; }
    
    double getEnergyLeft() const {
        return energyAllocated - energyUsed;
    }
    
    double getCurrentUsage() const {
        return energyUsed;
    }
    
    // Total amount owed across all unpaid bills
    double getTotalOwed() const {
        double total = 0.0;
        for (const auto& payment : paymentHistory) {
            if (!payment.isPaid) {
                total += payment.amount;
            }
        }
        return total;
    }
    
    // Check if any bills are overdue
    bool hasOverdueBills() const {
        for (const auto& payment : paymentHistory) {
            if (payment.isOverdue()) {
                return true;
            }
        }
        return false;
    }
    
    // Print all customer info to console
    void printDetails() const {
        cout << "--- Customer Info ---\n"
             << "ID: " << customerID << "\n"
             << "Name: " << name << "\n"
             << "Province: " << province << "\n"
             << "Email: " << email << "\n"
             << "Address: " << address << "\n"
             << "Energy Type: " << getEnergyName(primaryEnergyType) << "\n"
             << "Allocation: " << energyAllocated << " units\n"
             << "Current Usage: " << energyUsed << " units\n"
             << "Remaining: " << getEnergyLeft() << " units\n\n";
        
        if (!paymentHistory.empty()) {
            cout << "Payment History:\n";
            for (size_t i = 0; i < paymentHistory.size(); i++) {
                const Payment& payment = paymentHistory[i];
                cout << "  Bill #" << i+1 << " (" << payment.formatDate() << "): $" 
                     << fixed << setprecision(2) << payment.amount
                     << " - " << (payment.isPaid ? "Paid" : "Unpaid") 
                     << " - " << payment.getDaysSince() << " days ago";
                
                if (payment.isOverdue()) {
                    cout << " (OVERDUE!)";
                }
                cout << "\n";
            }
        } else {
            cout << "No bills yet.\n";
        }
        
        if (!maintenanceHistory.empty()) {
            cout << "\nMaintenance Records:\n";
            for (const auto& record : maintenanceHistory) {
                struct tm *timeinfo = localtime(&record.date);
                char buffer[80];
                strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
                
                cout << "  " << buffer << ": " << record.description 
                     << " - Cost: $" << fixed << setprecision(2) << record.cost << "\n";
            }
        }
        
        cout << "\n";
    }
};

// Main system class that manages everything
class EnergySystem {
private:
    vector<Customer> customers;
    map<string, vector<int>> provinceMap; // Maps province to customer indices
    map<EnergyType, double> energyRates;  // Pricing for each energy type
    
    vector<ImportExport> importExportLog;
    
    // Helper function to pick a random energy type
    EnergyType randomEnergyType() {
        static vector<EnergyType> types = {
            EnergyType::CRUDE_OIL, 
            EnergyType::SOLAR, 
            EnergyType::NUCLEAR, 
            EnergyType::NATURAL_GAS
        };
        
        static random_device rd;
        static mt19937 gen(rd());
        uniform_int_distribution<> distrib(0, types.size() - 1);
        
        return types[distrib(gen)];
    }
    
    // Helper for random numbers
    double randomNumber(double min, double max) {
        static random_device rd;
        static mt19937 gen(rd());
        uniform_real_distribution<> distrib(min, max);
        
        return distrib(gen);
    }
    
    // Helper for picking a province
    string randomProvince() {
        static vector<string> provinces = {
            "Ontario", "Quebec", "Alberta", "British Columbia", "Manitoba"
        };
        
        static random_device rd;
        static mt19937 gen(rd());
        uniform_int_distribution<> distrib(0, provinces.size() - 1);
        
        return provinces[distrib(gen)];
    }

public:
    // Constructor - set up initial energy rates
    EnergySystem() {
        // These rates are per unit (kWh, barrel, etc.)
        energyRates[EnergyType::CRUDE_OIL] = 1.25;
        energyRates[EnergyType::SOLAR] = 0.18;
        energyRates[EnergyType::NUCLEAR] = 0.22;
        energyRates[EnergyType::NATURAL_GAS] = 0.85;
    }
    
    // Add a customer to the system
    void addCustomer(const Customer& customer) {
        customers.push_back(customer);
        provinceMap[customer.getProvince()].push_back(customers.size() - 1);
    }
    
    // Create test data - we need 500 customers total
    void createTestData() {
        vector<string> provinces = {
            "Ontario", "Quebec", "Alberta", "British Columbia", "Manitoba"
        };
        
        // First names for generating random customers
        vector<string> firstNames = {
            "John", "Jane", "Mike", "Emily", "Dave"
        };
        
        // Last names for random customers
        vector<string> lastNames = {
            "Smith", "Johnson", "Williams", "Brown", "Jones"
        };
        
        random_device rd;
        mt19937 gen(rd());
        
        int id = 1001; // Starting customer ID
        
        // Make 100 customers for each of our 5 provinces
        for (const string& province : provinces) {
            for (int i = 0; i < 100; i++) {
                // Pick random names
                uniform_int_distribution<> firstDist(0, firstNames.size() - 1);
                uniform_int_distribution<> lastDist(0, lastNames.size() - 1);
                string first = firstNames[firstDist(gen)];
                string last = lastNames[lastDist(gen)];
                string fullName = first + " " + last;
                
                // Make an email - first initial + last name
                string email = first.substr(0, 1) + last + "@email.com";
                transform(email.begin(), email.end(), email.begin(), ::tolower);
                
                // Random address
                uniform_int_distribution<> streetNumDist(1, 9999);
                uniform_int_distribution<> streetDist(0, 15);
                vector<string> streets = {
                    "Howard Ave", "Dougall Ave", "Walker Rd", "Ouellette Ave", "Lauzon Rd"
                };
                string address = to_string(streetNumDist(gen)) + " " + streets[streetDist(gen)] + ", " + province;
                
                // Assign random energy type and usage
                EnergyType type = randomEnergyType();
                double allocation = randomNumber(250.0, 1000.0);
                
                // Create customer
                Customer customer(id++, fullName, province, email, address, type, allocation);
                
                // Add some random energy usage
                customer.useEnergy(randomNumber(50.0, allocation * 0.8));
                
                // Some customers have bills (every 3rd one)
                if (i % 3 == 0) {
                    customer.createBill(energyRates[type]);
                    
                    // Make some bills overdue (every 9th one)
                    if (i % 9 == 0) {
                        // Just don't pay this one - we'll pretend it's overdue
                    } else {
                        // Pay bill for others
                        customer.makePayment(0, customer.getTotalOwed());
                    }
                }
                
                // Add maintenance records to some customers
                if (i % 10 == 0) {
                    customer.addMaintenance("Equipment check", randomNumber(50.0, 200.0));
                }
                if (i % 25 == 0) {
                    customer.addMaintenance("Major system upgrade", randomNumber(500.0, 2000.0));
                }
                
                // Add to our system
                addCustomer(customer);
            }
        }
        
        // Create some import/export entries
        for (int i = 0; i < 50; i++) {
            EnergyType type = randomEnergyType();
            double quantity = randomNumber(1000.0, 10000.0);
            double price = randomNumber(energyRates[type] * 0.7, energyRates[type] * 1.3);
            bool isImport = (i % 3 != 0); // 2/3 are imports, 1/3 exports
            
            importExportLog.push_back(ImportExport(type, quantity, price, isImport));
        }
    }
    
    // Add a new import/export record
    void addImportExport(const ImportExport& record) {
        importExportLog.push_back(record);
    }
    
    // Process billing for all customers
    void doBilling() {
        for (auto& customer : customers) {
            double rate = energyRates[customer.getEnergyType()];
            if (customer.getCurrentUsage() > 0) {
                customer.createBill(rate);
            }
        }
    }
    
    // Send reminders to customers with overdue bills
    void sendReminders() {
        for (auto& customer : customers) {
            if (customer.hasOverdueBills()) {
                string emailText = customer.sendReminder();
                if (!emailText.empty()) {
                    // In real life we'd actually send the email here
                    cout << "Sent reminder to " << customer.getName() 
                         << " (ID: " << customer.getID() << ")\n";
                }
            }
        }
    }
    
    // Get stats broken down by province
    map<string, map<string, double>> getProvinceStats() {
        map<string, map<string, double>> stats;
        
        // Set up initial stats for each province
        for (const auto& pair : provinceMap) {
            const string& province = pair.first;
            stats[province]["customers"] = pair.second.size();
            stats[province]["allocated"] = 0;
            stats[province]["used"] = 0;
            stats[province]["unpaid"] = 0;
            stats[province]["overdue"] = 0;
        }
        
        // Fill in the stats
        for (const auto& pair : provinceMap) {
            const string& province = pair.first;
            const vector<int>& indices = pair.second;
            
            for (int index : indices) {
                const Customer& customer = customers[index];
                
                stats[province]["allocated"] += customer.getEnergyLeft() + customer.getCurrentUsage();
                stats[province]["used"] += customer.getCurrentUsage();
                stats[province]["unpaid"] += customer.getTotalOwed();
                
                if (customer.hasOverdueBills()) {
                    stats[province]["overdue"] += 1;
                }
            }
        }
        
        return stats;
    }
    
    // Create a monthly report file
    void createMonthlyReport(const string& filename) {
        ofstream report(filename);
        
        if (!report.is_open()) {
            cerr << "Couldn't open report file: " << filename << endl;
            return;
        }
        
        // Current date for the report
        time_t now = time(nullptr);
        struct tm *timeinfo = localtime(&now);
        char dateBuffer[80];
        strftime(dateBuffer, sizeof(dateBuffer), "%B %Y", timeinfo);
        
        report << "Energy Provider Monthly Report - " << dateBuffer << "\n\n";
        
        // Overall stats
        report << "Overall Stats:\n";
        report << "Total Customers: " << customers.size() << "\n";
        
        double totalUnpaid = 0.0;
        int overdueCount = 0;
        
        for (const auto& customer : customers) {
            totalUnpaid += customer.getTotalOwed();
            if (customer.hasOverdueBills()) {
                overdueCount++;
            }
        }
        
        report << "Total Unpaid: $" << fixed << setprecision(2) << totalUnpaid << "\n";
        report << "Overdue Customers: " << overdueCount 
               << " (" << fixed << setprecision(1) 
               << (static_cast<double>(overdueCount) / customers.size() * 100.0) 
               << "%)\n\n";
        
        // Stats by province
        report << "Province Breakdown:\n";
        auto provinceStats = getProvinceStats();
        
        for (const auto& pair : provinceStats) {
            const string& province = pair.first;
            const auto& stats = pair.second;
            
            report << province << ":\n";
            report << "  Customers: " << static_cast<int>(stats.at("customers")) << "\n";
            report << "  Energy Allocated: " << fixed << setprecision(2) << stats.at("allocated") << " units\n";
            report << "  Energy Used: " << fixed << setprecision(2) << stats.at("used") 
                   << " (" << (stats.at("used") / stats.at("allocated") * 100.0) << "%)\n";
            report << "  Unpaid Bills: $" << fixed << setprecision(2) << stats.at("unpaid") << "\n";
            report << "  Overdue Customers: " << static_cast<int>(stats.at("overdue")) 
                   << " (" << (stats.at("overdue") / stats.at("customers") * 100.0) << "%)\n\n";
        }
        
        // Import/Export info
        double totalImports = 0.0;
        double totalExports = 0.0;
        map<EnergyType, double> importsByType;
        map<EnergyType, double> exportsByType;
        
        for (const auto& record : importExportLog) {
            double value = record.getTotalValue();
            
            if (record.isImport) {
                totalImports += value;
                importsByType[record.energyType] += value;
            } else {
                totalExports += value;
                exportsByType[record.energyType] += value;
            }
        }
        
        report << "Import/Export Summary:\n";
        report << "Total Imports: $" << fixed << setprecision(2) << totalImports << "\n";
        report << "Total Exports: $" << fixed << setprecision(2) << totalExports << "\n";
        report << "Net Balance: $" << fixed << setprecision(2) << (totalImports - totalExports) << "\n\n";
        
        report << "Imports by Type:\n";
        for (const auto& pair : importsByType) {
            report << "  " << getEnergyName(pair.first) << ": $" 
                   << fixed << setprecision(2) << pair.second << "\n";
        }
        
        report << "\nExports by Type:\n";
        for (const auto& pair : exportsByType) {
            report << "  " << getEnergyName(pair.first) << ": $" 
                   << fixed << setprecision(2) << pair.second << "\n";
        }
        
        report << "\n--- End of Report ---\n";
        report.close();
        
        cout << "Report saved to " << filename << endl;
    }
    
    // Search for customers
    vector<Customer*> findCustomers(const string& searchText, const string& province = "") {
        vector<Customer*> results;
        
        for (auto& customer : customers) {
            // Skip if province doesn't match (when specified)
            if (!province.empty() && customer.getProvince() != province) {
                continue;
            }
            
            // Match ID, name or email
            if (to_string(customer.getID()).find(searchText) != string::npos ||
                customer.getName().find(searchText) != string::npos ||
                customer.getEmail().find(searchText) != string::npos) {
                results.push_back(&customer);
            }
        }
        
        return results;
    }
    
    // Get list of customers with overdue bills
    vector<Customer*> getOverdueCustomers() {
        vector<Customer*> results;
        
        for (auto& customer : customers) {
            if (customer.hasOverdueBills()) {
                results.push_back(&customer);
            }
        }
        
        return results;
    }
    
    // Show general system statistics
    void showStats() {
        cout << "+++ Energy Provider System Stats +++\n";
        cout << "Total Customers: " << customers.size() << "\n\n";
        
        cout << "By Province:\n";
        for (const auto& pair : provinceMap) {
            cout << "  " << pair.first << ": " << pair.second.size() << " customers\n";
        }
        
        cout << "\nEnergy Rates:\n";
        for (const auto& pair : energyRates) {
            cout << "  " << getEnergyName(pair.first) << ": $" 
                 << fixed << setprecision(2) << pair.second << " per unit\n";
        }
        
        // Overdue stats
        int overdueCount = 0;
        double overdueAmount = 0.0;
        
        for (const auto& customer : customers) {
            if (customer.hasOverdueBills()) {
                overdueCount++;
                overdueAmount += customer.getTotalOwed();
            }
        }
        
        cout << "\nOverdue Payments:\n";
        cout << "  Customers with overdue bills: " << overdueCount 
             << " (" << fixed << setprecision(1) 
             << (static_cast<double>(overdueCount) / customers.size() * 100.0) 
             << "%)\n";
        cout << "  Total overdue amount: $" << fixed << setprecision(2) << overdueAmount << "\n";
        
        // Import/Export numbers
        double importTotal = 0.0;
        double exportTotal = 0.0;
        
        for (const auto& record : importExportLog) {
            if (record.isImport) {
                importTotal += record.getTotalValue();
            } else {
                exportTotal += record.getTotalValue();
            }
        }
        
        cout << "\nImport/Export:\n";
        cout << "  Total imports: $" << fixed << setprecision(2) << importTotal << "\n";
        cout << "  Total exports: $" << fixed << setprecision(2) << exportTotal << "\n";
        cout << "  Balance: $" << fixed << setprecision(2) 
             << (importTotal - exportTotal) << "\n";
        
        cout << "\n";
    }
};

// Simple menu system
void showMenu(EnergySystem& system) {
    int choice;
    
    do {
        cout << "\n===== Energy Provider System =====\n";
        cout << "1. Find customers\n";
        cout << "2. Show overdue customers\n";
        cout << "3. Send payment reminders\n";
        cout << "4. Run billing process\n";
        cout << "5. View system stats\n";
        cout << "6. Generate monthly report\n";
        cout << "0. Exit\n";
        cout << "Your choice: ";
        cin >> choice;
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear input buffer
        
        string query, province;
        vector<Customer*> results;
        
        switch(choice) {
            case 1: // Search for customers
                cout << "Search (name, ID, or email): ";
                getline(cin, query);
                
                cout << "Filter by province (optional): ";
                getline(cin, province);
                
                results = system.findCustomers(query, province);
                
                cout << "\nFound " << results.size() << " customers:\n";
                for (auto* customer : results) {
                    customer->printDetails();
                    cout << "-------------------------\n";
                }
                break;
                
            case 2: // Show overdue customers
                results = system.getOverdueCustomers();
                
                cout << "\nFound " << results.size() << " customers with overdue bills:\n";
                for (auto* customer : results) {
                    customer->printDetails();
                    cout << "-------------------------\n";
                }
                break;
                
            case 3: // Send reminders
                system.sendReminders();
                cout << "Payment reminders have been sent!\n";
                break;
                
            case 4: // Process billing
                system.doBilling();
                cout << "Billing completed for all customers.\n";
                break;
                
            case 5: // View statistics
                system.showStats();
                break;
                
            case 6: // Generate report
                system.createMonthlyReport("monthly_report.txt");
                break;
                
            case 0: // Exit
                cout << "Thanks for using the Energy Provider System!\n";
                break;
                
            default:
                cout << "Oops! Invalid option. Try again.\n";
        }
        
    } while (choice != 0);
}

int main() {
    // Create our system
    EnergySystem energySystem;
    
    // Generate some test data
    cout << "Setting up test data...\n";
    energySystem.createTestData();
    cout << "Done! 500 customers created in 5 provinces.\n";
    
    // Show the menu
    showMenu(energySystem);
    
    return 0;
}