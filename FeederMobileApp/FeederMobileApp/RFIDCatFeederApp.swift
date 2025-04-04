import SwiftUI
import SwiftData

@main
struct RFIDCatFeederApp: App {
    @State private var feeders: [Feeder] = []
    
    var sharedModelContainer: ModelContainer = {
        let schema = Schema([
            Item.self,
        ])
        let modelConfiguration = ModelConfiguration(schema: schema, isStoredInMemoryOnly: false)
        
        do {
            return try ModelContainer(for: schema, configurations: [modelConfiguration])
        } catch {
            fatalError("Could not create ModelContainer: \(error)")
        }
    }()
    
    var body: some Scene {
        WindowGroup {
            NavigationStack {
                ZStack
                {
                    FeederDashboardView(feeders: feeders, updateFeeder: updateFeeder, removeFeeder: removeFeeder)
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                        .onAppear
                    {
                        loadFeeders()
                    }
                }
                .toolbar {
                    NavigationLink(destination: AddFeederView(feeders: $feeders, updateFeeder: updateFeeder)) {
                        HStack {
                            Text("Add new")
                            Image(systemName: "plus.circle.fill")
                        }
                    }
                    .padding(30)
                }
            }
        }
    }
    
    private func loadFeeders() {
        if let savedData = UserDefaults.standard.data(forKey: "savedFeeders") {
            if let decodedFeeders = try? JSONDecoder().decode([Feeder].self, from: savedData) {
                feeders = decodedFeeders
            }
        }
    }
    
    private func removeFeeder(_ feeder: Feeder) {
        feeders.removeAll { $0.id == feeder.id }
        saveFeeders()
    }
    
    private func saveFeeders() {
        if let encoded = try? JSONEncoder().encode(feeders) {
            UserDefaults.standard.set(encoded, forKey: "savedFeeders")
        }
    }
    
    func updateFeeder(feeder : Feeder, updateDB: Bool)
    {
        if let index = feeders.firstIndex(where: { $0.id == feeder.id })
        {
            // update feeder
            feeders[index] = feeder
            
            // save the feeders on disk memory
            saveFeeders()
            
            // Consider doing an http request here to also update the entries from the DB
            if (updateDB)
            {
                updateFeederInDB(feeder: feeder) { success, message in
                    if success {
                        print("Success: \(message ?? "Feeder updated.")")
                    } else {
                        print("Failed: \(message ?? "An unknown error occurred.")")
                    }
                }
            }
        }
    }
    
    func updateFeederInDB(feeder: Feeder, completion: @escaping (Bool, String?) -> Void)
    {
        // Define the API URL
        guard let url = URL(string: "https://dev.bull-software.com/update_feeder.php") else {
            completion(false, "Invalid URL.")
            return
        }
        
        // Create the JSON payload
        let payload: [String: Any] = [
            "ID": feeder.id,
            "Password": feeder.password,
            "TrapMode": feeder.trapMode,
            "FeedFoodConfiguration": feeder.feedConfig,
            "FoodStorageQuantity": feeder.foodStorageQuantity,
            "LastFoodStorageQuantityUpdateTime": feeder.lastFoodStorageQuantityUpdateTime,
            "FoodCurrentWeight": feeder.foodCurrentWeight,
            "LastFoodCurrentWeightUpdateTime": feeder.lastFoodCurrentWeightUpdateTime
        ]
        
        // Convert the payload to JSON
        guard let jsonData = try? JSONSerialization.data(withJSONObject: payload, options: []) else {
            completion(false, "Failed to encode payload.")
            return
        }
        
        // Create the PUT request
        var request = URLRequest(url: url)
        request.httpMethod = "PUT"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = jsonData
        
        // Perform the network request
        URLSession.shared.dataTask(with: request) { data, response, error in
            if let error = error {
                completion(false, "Request failed: \(error.localizedDescription)")
                return
            }
            
            guard let httpResponse = response as? HTTPURLResponse else {
                completion(false, "Invalid response.")
                return
            }
            
            // Handle the server's response
            switch httpResponse.statusCode {
            case 200...299:
                // Parse the response data if needed
                if let data = data,
                   let json = try? JSONSerialization.jsonObject(with: data, options: []) as? [String: Any],
                   let message = json["message"] as? String {
                    completion(true, message)
                } else {
                    completion(true, "Feeder updated successfully.")
                }
            case 400...499:
                if let data = data,
                   let json = try? JSONSerialization.jsonObject(with: data, options: []) as? [String: Any],
                   let errorMessage = json["error"] as? String {
                    completion(false, errorMessage)
                } else {
                    completion(false, "Client error: \(httpResponse.statusCode).")
                }
            case 500...599:
                completion(false, "Server error: \(httpResponse.statusCode).")
            default:
                completion(false, "Unexpected response code: \(httpResponse.statusCode).")
            }
        }.resume()
    }
}

struct FeederDashboardView: View {
    let feeders: [Feeder]
    var updateFeeder: (Feeder, Bool) -> Void
    var removeFeeder: (Feeder) -> Void
    
    var body: some View {
        ScrollView {
            VStack(spacing: 16) {
                if feeders.isEmpty {
                    Text("No Feeders Available")
                        .font(.title)
                        .foregroundColor(.gray)
                    
                    Text("Add a new feeder from the top right corner")
                        .font(.subheadline)
                        .foregroundColor(.gray)
                }
                else
                {
                    ForEach(feeders) { feeder in
                        NavigationLink(destination: FeederView(feeder: feeder, updateFeeder: updateFeeder)) {
                            FeederCardView(feeder: feeder, updateFeeder: updateFeeder, removeFeeder: removeFeeder)
                        }
                    }
                }
            }
            .frame(maxWidth: .infinity)
            .padding()
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

struct FeederCardView: View {
    @State var feeder: Feeder
    @State private var allFoodEvents: [FoodEvent] = []
    @State private var allGateEvents: [GateEvent] = []
    @State private var feederToDelete: Feeder? = nil
    @State private var showAlert = false
    @State private var timer: Timer?
    var updateFeeder: (Feeder, Bool) -> Void
    var removeFeeder: (Feeder) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            
            HStack {
                Text(feeder.name)
                    .font(.title2)
                    .fontWeight(.bold)
                
                Spacer()
                
                Image(systemName: getFeederConnectionStatus() ? "wifi" : "wifi.slash")
                    .foregroundColor(getFeederConnectionStatus() ? .green : .red)
                    .font(.title2)
                
                Button(action: {
                    feederToDelete = feeder
                    showAlert = true
                }) {
                    Image(systemName: "trash")
                        .foregroundColor(.red)
                        .padding(8)
                        .background(Color.red.opacity(0.1))
                        .clipShape(Circle())
                }
                .buttonStyle(BorderlessButtonStyle())
            }
            
            Divider()
            
            HStack {
                Text("Current Weight:")
                    .font(.headline)
                    .foregroundColor(.primary)
                
                Text("\(getFeederCurrentWeight(), specifier: "%.0f") gr")
                    .font(.headline)
                    .fontWeight(.semibold)
                    .foregroundColor(.secondary)
                
                Text("Updated " + timeDifference(from: feeder.lastFoodCurrentWeightUpdateTime))
                    .font(.subheadline)
                    .foregroundColor(.gray)
                
                Spacer()
            }
            
            HStack {
                Text("Remaining Food:")
                    .font(.headline)
                    .foregroundColor(.primary)
                
                Text("\(getRemainingFoodWeight(), specifier: "%.1f") kg")
                    .font(.headline)
                    .fontWeight(.semibold)
                    .foregroundColor(.secondary)
                
                Text("Refilled " + timeDifference(from: feeder.lastFoodStorageQuantityUpdateTime))
                    .font(.subheadline)
                    .foregroundColor(.gray)
                
                Spacer()
            }
        }
        .padding(16)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color(.systemGray6))
                .shadow(color: Color.black.opacity(0.1), radius: 5, x: 0, y: 3)
        )
        .padding(.horizontal)
        .onAppear { startFetching() }
        .onDisappear { stopFetching() }
        .alert("Are you sure you want to delete this feeder?", isPresented: $showAlert) {
            Button("Cancel", role: .cancel) {}
            Button("Delete", role: .destructive) {
                if let feederToDelete = feederToDelete {
                    removeFeeder(feederToDelete)
                }
            }
        }
    }
    
    func timeDifference(from unixTime: Int) -> String
    {
        let currentTime = Date().timeIntervalSince1970
        let difference = Int(currentTime) - unixTime
        
        if difference < 60 {
            return "\(difference) second\(difference == 1 ? "" : "s") ago"
        } else if difference < 3600 {
            let minutes = difference / 60
            return "\(minutes) minute\(minutes == 1 ? "" : "s") ago"
        } else if difference < 86400 {
            let hours = difference / 3600
            return "\(hours) hour\(hours == 1 ? "" : "s") ago"
        } else {
            let days = difference / 86400
            return "\(days) day\(days == 1 ? "" : "s") ago"
        }
    }
    
    
    func startFetching() {
        let interval = 10.0 //refresh on each 10 seconds
        
        timer = Timer.scheduledTimer(withTimeInterval: interval, repeats: true) { _ in
            fetchFeederFullData()
        }
    }
    
    func stopFetching() {
        timer?.invalidate()  // Stop the timer when the view disappears
    }
    
    func fetchFeederFullData()
    {
        fetchFeeder()
        fetchGateEvents()
        fetchFoodDispenseEvents()
    }
    
    func fetchFeeder()
    {
        guard let url = URL(string: "https://dev.bull-software.com/get_feeder.php?ID=\(feeder.id)&Password=\(feeder.password)") else
        {
            return
        }
        
        URLSession.shared.dataTask(with: url) { data, response, error in
            DispatchQueue.main.async {
                
                guard let data = data else {
                    return
                }
                
                do
                {
                    let updatedFeederData = try JSONDecoder().decode(FeederResponse.self, from: data)
                    self.feeder = Feeder(
                        id: updatedFeederData.ID,
                        password: feeder.password,
                        name: updatedFeederData.Name,
                        trapMode: updatedFeederData.TrapMode,
                        feedConfig: updatedFeederData.FeedFoodConfiguration,
                        foodStorageQuantity: updatedFeederData.FoodStorageQuantity,
                        lastFoodStorageQuantityUpdateTime: updatedFeederData.LastFoodStorageQuantityUpdateTime,
                        foodCurrentWeight: updatedFeederData.FoodCurrentWeight,
                        lastFoodCurrentWeightUpdateTime: updatedFeederData.LastFoodCurrentWeightUpdateTime
                    )
                    
                    print(self.feeder);
                    
                    // cache the feeder data into memory
                    updateFeeder(feeder, false)
                }
                catch
                {
                    
                }
            }
        }.resume()
    }
    
    private func fetchFoodDispenseEvents()
    {
        guard let url = URL(string: "https://dev.bull-software.com/get_food_dispense_events.php?ID=\(feeder.id)&Password=\(feeder.password)") else {
            return
        }
        
        URLSession.shared.dataTask(with: url) { data, _, error in
            DispatchQueue.main.async {
                
                guard let data = data else {
                    return
                }
                
                do {
                    let allEvents = try JSONDecoder().decode([FoodEvent].self, from: data)
                    
                    self.allFoodEvents = allEvents
                }
                catch
                {
                }
            }
        }.resume()
    }
    
    private func fetchGateEvents() {
        guard let url = URL(string: "https://dev.bull-software.com/get_gate_events.php?ID=\(feeder.id)&Password=\(feeder.password)") else {
            return
        }
        
        URLSession.shared.dataTask(with: url) { data, _, error in
            DispatchQueue.main.async {
                
                guard let data = data else
                {
                    return
                }
                
                do {
                    let allEvents = try JSONDecoder().decode([GateEvent].self, from: data)
                    
                    self.allGateEvents = allEvents
                }
                catch
                {
                    
                }
            }
        }.resume()
    }
    
    func getFeederConnectionStatus() -> Bool
    {
        let isConnectionActiveLimit = 1200 // 20 minutes in seconds
        
        if(!isUnixTimeExceededLimit(lastUpdate: feeder.lastFoodCurrentWeightUpdateTime, limit: isConnectionActiveLimit))
        {
            return true;
        }
        
        let gateEventsHasNotExceeded = allGateEvents.contains
        { event in
            !isUnixTimeExceededLimit(lastUpdate: event.endTime, limit: isConnectionActiveLimit)
        }
        
        if(gateEventsHasNotExceeded)
        {
            return true;
        }
        
        let dispenseEventsHasNotExceeded = allFoodEvents.contains
        { event in
            !isUnixTimeExceededLimit(lastUpdate: event.dispensedAt, limit: isConnectionActiveLimit)
        }
        
        if(dispenseEventsHasNotExceeded)
        {
            return true;
        }
        
        return false;
    }
    
    func getFeederCurrentWeight() -> Double {
        return feeder.foodCurrentWeight
    }
    
    func getRemainingFoodWeight() -> Double
    {
        let storageQuantity = feeder.foodStorageQuantity
        let storageQuantityUpdatedTime = feeder.lastFoodStorageQuantityUpdateTime
        
        // Filter events that occurred after the storage quantity was last updated
        let relevantEvents = allFoodEvents.filter { $0.dispensedAt > storageQuantityUpdatedTime }
        
        // Calculate the total quantity dispensed in the relevant events
        let totalDispensed = relevantEvents.reduce(0.0) { $0 + $1.quantityDispensed }
        
        // Subtract the total dispensed quantity from the storage quantity
        let remainingFood = storageQuantity - totalDispensed
        
        // Ensure the result is not negative
        return max(remainingFood, 0.0)
    }
    
    func isUnixTimeExceededLimit(lastUpdate: Int, limit: Int) -> Bool
    {
        let currentTime = Int(Date().timeIntervalSince1970) // Get current Unix timestamp
        let timeDifference = currentTime - lastUpdate
        
        return timeDifference >= limit // 20 minutes = 1200 seconds
    }
}
