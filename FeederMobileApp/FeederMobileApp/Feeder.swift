import Foundation

struct Feeder: Codable, Identifiable
{
    var id: String
    var password: String
    var name: String
    var trapMode: String
    var feedConfig: [String: Int]
    var foodStorageQuantity: Double
    var lastFoodStorageQuantityUpdateTime: Int
    var foodCurrentWeight: Double
    var lastFoodCurrentWeightUpdateTime: Int
}
