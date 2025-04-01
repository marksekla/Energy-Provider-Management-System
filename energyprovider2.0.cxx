/*
 * Energy Provider System - COMP-3400 Project
*/

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <algorithm>
#include <limits>

using namespace std;

// Energy types our company provides
enum class EnergyType { CRUDE_OIL, SOLAR, NUCLEAR, NATURAL_GAS };

// Convert energy type to readable string - needed for reports
string getEnergyName(EnergyType t) {
    switch (t) {
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
    
    Payment(double amt) : amount(amt), isPaid(false), date(time(nullptr)) {}
    
    // How many days since we sent the bill
    int getDaysSince() const { 
        return difftime(time(nullptr), date) / (60*60*24); 
    }
    
    // Past 30 days and still not paid? It's overdue
    bool isOverdue() const { 
        return getDaysSince() > 30 && !isPaid; 
    }
    
    // Nicer date format for printing
    string formatDate() const {
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", localtime(&date));
        return string(buffer);
    }
};

// For tracking our imports and exports
struct ImportExport {
    EnergyType type;
    double quantity, price;  // units depend on energy type
    bool isImport;           // true=import, false=export
    time_t date;
    
    ImportExport(EnergyType t, double q, double p, bool imp) 
        : type(t), quantity(q), price(p), isImport(imp), date(time(nullptr)) {}
    
    // Total value of this transaction
    double getValue() const { return quantity * price; }
};

// Our customer class
class Customer {
private:
    int id;
    string name, province, email, address;
    EnergyType energyType;
    double allocated, used = 0;   // How much they're allowed to use & used so far
    vector<Payment> payments;
    bool reminderSent = false;
    
    // For tracking maintenance stuff
    struct MaintRecord { 
        time_t date; 
        string desc; 
        double cost; 
    };
    vector<MaintRecord> maintenance;
    
public:
    Customer(int id, string name, string prov, string mail, string addr, EnergyType type, double alloc)
        : id(id), name(name), province(prov), email(mail), address(addr), 
          energyType(type), allocated(alloc) {}
          
    // Record energy usage - returns false if they try to use too much
    bool useEnergy(double amt) {
        if (amt <= allocated - used) {
            used += amt;
            return true;
        }
        return false;
    }
    
    // Create a new bill based on current usage
    void createBill(double rate) {
        payments.push_back(Payment(used * rate));
        used = 0; // Reset for next month
    }
    
    // Process a payment for a specific bill
    bool makePayment(int index, double amt) {
        if (index >= 0 && index < payments.size() && amt >= payments[index].amount) {
            payments[index].isPaid = true;
            reminderSent = false;
            return true;
        }
        return false;
    }
    
    // Add maintenance work to customer's record
    void addMaintenance(string desc, double cost) {
        maintenance.push_back({time(nullptr), desc, cost});
    }
    
    // Generate email reminder text for overdue bills
    string sendReminder() {
        if (hasOverdue() && !reminderSent) {
            reminderSent = true;
            stringstream email;
            email << "To: " << this->email << "\n"
                  << "Subject: Your energy payment is overdue\n\n"
                  << "Hi " << name << ",\n\n"
                  << "Just a reminder that you have unpaid bills that are now overdue:\n\n";
            
            for (auto& p : payments) {
                if (p.isOverdue()) {
                    email << "Bill from " << p.formatDate()
                          << " - Amount: $" << fixed << setprecision(2) << p.amount
                          << " - " << p.getDaysSince() - 30 << " days overdue\n";
                }
            }
            
            email << "\nPlease pay ASAP to avoid service interruption.\n\n"
                  << "Thanks,\nCustomer Service Team";
            
            return email.str();
        }
        return "";
    }
    
    // Total amount owed across all unpaid bills
    double getTotalOwed() const {
        double total = 0;
        for (auto& p : payments)
            if (!p.isPaid) total += p.amount;
        return total;
    }
    
    // Check if any bills are overdue
    bool hasOverdue() const {
        for (auto& p : payments)
            if (p.isOverdue()) return true;
        return false;
    }
    
    // Print all customer info to console
    void printDetails() const {
        cout << "--- Customer Info ---\n"
             << "ID: " << id << "\nName: " << name
             << "\nProvince: " << province
             << "\nEmail: " << email
             << "\nAddress: " << address
             << "\nEnergy Type: " << getEnergyName(energyType)
             << "\nAllocation: " << allocated << " units"
             << "\nCurrent Usage: " << used << " units"
             << "\nRemaining: " << (allocated - used) << " units\n\n";
        
        if (!payments.empty()) {
            cout << "Payment History:\n";
            for (size_t i = 0; i < payments.size(); i++) {
                cout << "  Bill #" << i+1 << " (" << payments[i].formatDate() << "): $" 
                     << fixed << setprecision(2) << payments[i].amount
                     << " - " << (payments[i].isPaid ? "Paid" : "Unpaid") 
                     << " - " << payments[i].getDaysSince() << " days ago";
                
                if (payments[i].isOverdue())
                    cout << " (OVERDUE!)";
                cout << "\n";
            }
        } else {
            cout << "No bills yet.\n";
        }
        
        if (!maintenance.empty()) {
            cout << "\nMaintenance Records:\n";
            for (auto& m : maintenance) {
                char buffer[80];
                strftime(buffer, sizeof(buffer), "%Y-%m-%d", localtime(&m.date));
                cout << "  " << buffer << ": " << m.desc 
                     << " - Cost: $" << fixed << setprecision(2) << m.cost << "\n";
            }
        }
        cout << "\n";
    }
    
    // Various getters
    int getID() const { return id; }
    string getName() const { return name; }
    string getEmail() const { return email; }
    string getProvince() const { return province; }
    EnergyType getEnergyType() const { return energyType; }
    double getUsed() const { return used; }
    double getAllocated() const { return allocated; }
};

// Main system class that manages everything
class EnergySystem {
private:
    vector<Customer> customers;
    map<string, vector<int>> provinces; // Maps province to customer indices
    map<EnergyType, double> rates;      // Pricing for each energy type
    vector<ImportExport> trades;
    mt19937 rng{random_device{}()};
    
    // Helper for random numbers
    double randNum(double min, double max) {
        return uniform_real_distribution<>(min, max)(rng);
    }
    
    // Helper function to pick a random energy type
    EnergyType randType() {
        vector<EnergyType> types = {
            EnergyType::CRUDE_OIL, EnergyType::SOLAR, 
            EnergyType::NUCLEAR, EnergyType::NATURAL_GAS
        };
        return types[uniform_int_distribution<>(0, 3)(rng)];
    }

public:
    // Constructor - set up initial energy rates
    EnergySystem() {
        // These rates are per unit (kWh, barrel, etc.)
        rates = {
            {EnergyType::CRUDE_OIL, 1.25},
            {EnergyType::SOLAR, 0.18},
            {EnergyType::NUCLEAR, 0.22},
            {EnergyType::NATURAL_GAS, 0.85}
        };
    }
    
    // Add a customer to the system
    void addCustomer(const Customer& c) {
        customers.push_back(c);
        provinces[c.getProvince()].push_back(customers.size() - 1);
    }
    
    // Create test data - we need 500 customers total
    void createTestData() {
        vector<string> provs = {"Ontario", "Quebec", "Alberta", "British Columbia", "Manitoba"};
        vector<string> fnames = {"John", "Jane", "Mike", "Emily", "Dave"};
        vector<string> lnames = {"Smith", "Johnson", "Williams", "Jones", "Brown"};
        vector<string> streets = {"Howard Ave", "Dougall Ave", "Walker Rd", "Ouellette Ave", "Lauzon Rd"};
        
        int id = 1001;
        
        // Make 100 customers for each of our 5 provinces
        for (auto& prov : provs) {
            for (int i = 0; i < 100; i++) {
                // Pick random names
                string first = fnames[uniform_int_distribution<>(0, fnames.size()-1)(rng)];
                string last = lnames[uniform_int_distribution<>(0, lnames.size()-1)(rng)];
                string name = first + " " + last;
                
                // Make an email - first initial + last name
                string email = first.substr(0,1) + last + "@email.com";
                transform(email.begin(), email.end(), email.begin(), ::tolower);
                
                // Random address
                string street = streets[uniform_int_distribution<>(0, streets.size()-1)(rng)];
                string address = to_string(uniform_int_distribution<>(100, 9999)(rng)) + " " + street + ", " + prov;
                
                // Assign random energy type and usage
                EnergyType type = randType();
                double alloc = randNum(250, 1000);
                
                Customer cust(id++, name, prov, email, address, type, alloc);
                
                // Add some random energy usage
                cust.useEnergy(randNum(50, alloc * 0.8));
                
                // Some customers have bills (every 3rd one)
                if (i % 3 == 0) {
                    cust.createBill(rates[type]);
                    
                    // Make some bills overdue (every 9th one)
                    if (i % 9 != 0) {
                        // Pay bill for others
                        cust.makePayment(0, cust.getTotalOwed());
                    }
                }
                
                // Add maintenance records to some customers
                if (i % 15 == 0) {
                    cust.addMaintenance("Equipment check", randNum(50, 200));
                }
                
                // Add to our system
                addCustomer(cust);
            }
        }
        
        // Create some import/export entries
        for (int i = 0; i < 30; i++) {
            EnergyType type = randType();
            double qty = randNum(1000, 10000);
            double price = randNum(rates[type] * 0.7, rates[type] * 1.3);
            trades.push_back(ImportExport(type, qty, price, i % 3 != 0)); // 2/3 are imports, 1/3 exports
        }
    }
    
    // Process billing for all customers
    void doBilling() {
        for (auto& c : customers)
            if (c.getUsed() > 0)
                c.createBill(rates[c.getEnergyType()]);
    }
    
    // Send reminders to customers with overdue bills
    void sendReminders() {
        int sent = 0;
        for (auto& c : customers) {
            string email = c.sendReminder();
            if (!email.empty()) {
                // In real life we'd actually send the email here
                cout << "Sent reminder to " << c.getName() << " (ID: " << c.getID() << ")\n";
                sent++;
            }
        }
    }
    
    // Create a monthly report file
    void createMonthlyReport(const string& filename) {
        ofstream report(filename);
        if (!report) {
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
        double totalUnpaid = 0;
        int overdueCount = 0;
        for (auto& c : customers) {
            totalUnpaid += c.getTotalOwed();
            if (c.hasOverdue()) overdueCount++;
        }
        
        report << "Overall Stats:\n"
               << "Total Customers: " << customers.size() << "\n"
               << "Total Unpaid: $" << fixed << setprecision(2) << totalUnpaid << "\n"
               << "Overdue Customers: " << overdueCount << " ("
               << fixed << setprecision(1) << (overdueCount * 100.0 / customers.size())
               << "%)\n\n";
        
        // Province breakdown
        report << "Province Breakdown:\n";
        for (auto& [prov, ids] : provinces) {
            double allocated = 0, used = 0, unpaid = 0;
            int overdue = 0;
            
            for (int idx : ids) {
                Customer& c = customers[idx];
                allocated += c.getAllocated();
                used += c.getUsed();
                unpaid += c.getTotalOwed();
                if (c.hasOverdue()) overdue++;
            }
            
            report << prov << ":\n"
                   << "  Customers: " << ids.size() << "\n"
                   << "  Energy Allocated: " << fixed << setprecision(2) << allocated << " units\n"
                   << "  Energy Used: " << used << " (" << (used/allocated*100) << "%)\n"
                   << "  Unpaid Bills: $" << unpaid << "\n"
                   << "  Overdue: " << overdue << " (" << (overdue*100.0/ids.size()) << "%)\n\n";
        }
        
        // Import/Export summary
        double imports = 0, exports = 0;
        map<EnergyType, double> importsByType;
        map<EnergyType, double> exportsByType;
        
        for (auto& t : trades) {
            if (t.isImport) {
                imports += t.getValue();
                importsByType[t.type] += t.getValue();
            } else {
                exports += t.getValue();
                exportsByType[t.type] += t.getValue();
            }
        }
        
        report << "Import/Export Summary:\n"
               << "Total Imports: $" << fixed << setprecision(2) << imports << "\n"
               << "Total Exports: $" << fixed << setprecision(2) << exports << "\n"
               << "Net Balance: $" << (imports - exports) << "\n\n";
               
        report << "Imports by Type:\n";
        for (auto& [type, value] : importsByType) {
            report << "  " << getEnergyName(type) << ": $" 
                   << fixed << setprecision(2) << value << "\n";
        }
        
        report << "\nExports by Type:\n";
        for (auto& [type, value] : exportsByType) {
            report << "  " << getEnergyName(type) << ": $" 
                   << fixed << setprecision(2) << value << "\n";
        }
               
        report << "\n--- End of Report ---\n";
        report.close();
        
        cout << "Report saved to " << filename << endl;
    }
    
    // Search for customers
    vector<Customer*> findCustomers(const string& query, const string& prov = "") {
        vector<Customer*> results;
        for (auto& c : customers) {
            // Skip if province doesn't match (when specified)
            if (!prov.empty() && c.getProvince() != prov) continue;
            
            // Match ID, name or email
            if (to_string(c.getID()).find(query) != string::npos ||
                c.getName().find(query) != string::npos ||
                c.getEmail().find(query) != string::npos) {
                results.push_back(&c);
            }
        }
        return results;
    }
    
    // Get list of customers with overdue bills
    vector<Customer*> getOverdueCustomers() {
        vector<Customer*> results;
        for (auto& c : customers)
            if (c.hasOverdue())
                results.push_back(&c);
        return results;
    }
    
    // Show general system statistics
    void showStats() {
        cout << "+++ Energy Provider System Stats +++\n";
        cout << "Total Customers: " << customers.size() << "\n\n";
        
        // By province
        cout << "By Province:\n";
        for (auto& [prov, list] : provinces)
            cout << "  " << prov << ": " << list.size() << " customers\n";
        
        // Energy rates
        cout << "\nEnergy Rates:\n";
        for (auto& [type, rate] : rates)
            cout << "  " << getEnergyName(type) << ": $" 
                 << fixed << setprecision(2) << rate << " per unit\n";
        
        // Overdue stats
        int overdueCount = 0;
        double overdueAmount = 0;
        for (auto& c : customers) {
            if (c.hasOverdue()) {
                overdueCount++;
                overdueAmount += c.getTotalOwed();
            }
        }
        
        cout << "\nOverdue Payments:\n";
        cout << "  Customers with overdue bills: " << overdueCount 
             << " (" << fixed << setprecision(1) 
             << (static_cast<double>(overdueCount) / customers.size() * 100.0) 
             << "%)\n";
        cout << "  Total overdue amount: $" << fixed << setprecision(2) << overdueAmount << "\n";
        
        // Import/Export numbers
        double importTotal = 0, exportTotal = 0;
        for (auto& t : trades) {
            if (t.isImport) importTotal += t.getValue();
            else exportTotal += t.getValue();
        }
        
        cout << "\nImport/Export:\n";
        cout << "  Total imports: $" << fixed << setprecision(2) << importTotal << "\n";
        cout << "  Total exports: $" << fixed << setprecision(2) << exportTotal << "\n";
        cout << "  Balance: $" << (importTotal - exportTotal) << "\n\n";
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
        cout << "\n";
        
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
                for (auto* c : results) {
                    c->printDetails();
                    cout << "-------------------------\n";
                }
                
                cout << "\nPress Enter to continue...";
                cin.get();
                break;
                
            case 2: // Show overdue customers
                results = system.getOverdueCustomers();
                
                cout << "Found " << results.size() << " customers with overdue bills:\n";
                for (auto* c : results) {
                    c->printDetails();
                    cout << "-------------------------\n";
                }
                
                cout << "\nPress Enter to continue...";
                cin.get();
                break;
                
            case 3: // Send reminders
                system.sendReminders();
                cout << "Payment reminders have been sent!\n";
                
                cout << "\nPress Enter to continue...";
                cin.get();
                break;
                
            case 4: // Process billing
                system.doBilling();
                cout << "Billing completed for all customers.\n";
                
                cout << "\nPress Enter to continue...";
                cin.get();
                break;
                
            case 5: // View statistics
                system.showStats();
                
                cout << "\nPress Enter to continue...";
                cin.get();
                break;
                
            case 6: // Generate report
                system.createMonthlyReport("monthly_report.txt");
                
                cout << "\nPress Enter to continue...";
                cin.get();
                break;
                
            case 0: // Exit
                cout << "Thanks for using the Energy Provider System!\n";
                break;
                
            default:
                cout << "Oops! Invalid option. Try again.\n";
                
                cout << "\nPress Enter to continue...";
                cin.get();
        }
        
    } while (choice != 0);
}

int main() {
    // Create our system
    EnergySystem system;
    
    // Generate some test data
    cout << "Setting up test data...\n";
    system.createTestData();
    cout << "Done! 500 customers created in 5 provinces.\n";
    
    // Show the menu
    showMenu(system);
    
    return 0;
}