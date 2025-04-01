# Energy Provider System – COMP-3400 Project

This project simulates a full-scale energy provider company that manages 500 customers across 5 provinces in Canada. It handles everything from energy consumption and billing to overdue reminders, maintenance tracking, import/export reporting, and system-wide statistics — all through a dynamic C++ console application.

---

## Features

### Customer Management
- Create and store customer profiles with name, province, address, energy type, and usage
- Track current energy usage, allocated limits, and maintenance records

### Billing System
- Generate monthly bills based on usage and energy type pricing
- Accept and track payments
- Automatically mark bills as overdue after 30 days
- Send overdue email-style reminders

### Monthly Reporting
- Generate detailed text reports for:
  - Total customers, overdue bills, and unpaid balances
  - Energy usage and allocation breakdowns by province
  - Import/export summaries by energy type
  - Net import/export balances

### Import/Export Tracking
- Records transactions of 4 energy types:
  - Crude Oil
  - Solar
  - Nuclear
  - Natural Gas
- Calculates total import/export values and net revenue

### Test Data Generation
- Creates 500 simulated customers
- Random names, emails, usage patterns, bills, and maintenance logs
- Assigns energy types and provinces at random
- Adds overdue and paid bills for realism

### Admin Tools & Search
- Search customers by name, ID, or email
- Filter by province
- View system-wide statistics
- Show list of overdue customers
- View detailed info per customer
- Process billing with one command

### File Output

- `monthly_report.txt`: Generated monthly summary with stats and breakdowns

---

## Sample Use Cases

- Track how much energy each customer has used or owes
- Automatically remind customers about overdue bills
- Generate a report showing energy trends across provinces
- Search for a specific customer and view their billing and maintenance history
- View how much energy was imported/exported and the resulting net gain/loss

---

## Output example

<img width="250" alt="Screenshot 2025-04-01 at 2 05 32 AM" src="https://github.com/user-attachments/assets/641c5352-843a-4652-879d-c8c0f2be1a7e" />

---

## Future Improvements

- Add file-based saving/loading for persistent data
- GUI or web interface for easier use
- Integration with real-time email notifications
- More dynamic energy pricing models
